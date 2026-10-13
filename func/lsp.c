//this is lsp.c
//talks to clangd (in tools/llvm-mingw) using the Language Server Protocol.
//every message, both ways, looks like
//    Content-Length: 123\r\n
//    \r\n
//    {"jsonrpc":"2.0", ...}      <- 123 bytes of json
//a request has an "id" and gets a reply with the same id. a notification doesn't.
//for now it only asks for semantic tokens: what each name in the file is.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "raylib.h"
#include "cJSON.h"
#include "lsp.h"
#include "app_spawn.h"
#include "toolchain.h"
#include "../ui/ui_textarea.h"

static int running = 0;  // clangd is up
static int ready = 0;    // it answered "initialize", so we can send files now
static int dead = 0;     // a write to clangd failed. stop on the next update
static int nextId = 1;
static int initId = 0;   // id of our initialize request
static int tokensId = 0; // id of the newest token request (replies to older ones are out of date)

static char *uri = NULL; // the open file, as file:///C:/...
static int cppFile = 0;
static int version = 0;  // goes up every time we send the text
static int changed = 0;
static double changedAt = 0;

static char *inbox = NULL; // bytes from clangd that aren't a whole message yet
static int inboxLen = 0, inboxCap = 0;

static LspToken *tokens = NULL;
static int tokenCount = 0;
static char **typeNames = NULL; // clangd's list of kinds. a token's type indexes this
static int typeCount = 0;

static char *copyString(const char *s) {
    size_t n = strlen(s) + 1;
    char *copy = malloc(n);
    if (copy) memcpy(copy, s, n);
    return copy;
}

// 0 = not code, 1 = c, 2 = c++
static int language(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return 0;
    char ext[8] = "";
    for (int i = 0; dot[i] && i < 7; i++) {
        ext[i] = tolower((unsigned char)dot[i]);
        ext[i + 1] = '\0';
    }
    if (!strcmp(ext, ".c") || !strcmp(ext, ".h")) return 1;
    if (!strcmp(ext, ".cpp") || !strcmp(ext, ".cc") || !strcmp(ext, ".cxx") || !strcmp(ext, ".c++") ||
        !strcmp(ext, ".hpp") || !strcmp(ext, ".hh") || !strcmp(ext, ".hxx")) return 2;
    return 0;
}

// C:\my dir\a.c -> file:///C:/my%20dir/a.c
static char *makeUri(const char *path) {
    char *out = malloc(8 + strlen(path) * 3 + 1);
    if (!out) return NULL;
    char *o = out + sprintf(out, "file:///");
    for (const unsigned char *p = (const unsigned char *)path; *p; p++) {
        if (*p == '\\') *o++ = '/';
        else if (isalnum(*p) || strchr("-._~/:", *p)) *o++ = *p;
        else o += sprintf(o, "%%%02X", *p);
    }
    *o = '\0';
    return out;
}

// the whole file as one string, lines joined with \n
static char *documentText(void) {
    size_t total = 1;
    for (int i = 0; i < lineCount; i++) total += lines[i].length + 1;
    char *text = malloc(total);
    if (!text) return NULL;
    size_t p = 0;
    for (int i = 0; i < lineCount; i++) {
        memcpy(text + p, lines[i].buffer, lines[i].length);
        p += lines[i].length;
        if (i < lineCount - 1) text[p++] = '\n';
    }
    text[p] = '\0';
    return text;
}

// sending

static void post(cJSON *msg) {
    cJSON_AddStringToObject(msg, "jsonrpc", "2.0");
    char *body = cJSON_PrintUnformatted(msg);
    cJSON_Delete(msg);
    if (!body) return;
    int len = (int)strlen(body);
    char header[64];
    int n = snprintf(header, sizeof(header), "Content-Length: %d\r\n\r\n", len);
    if (!writeServer(header, n) || !writeServer(body, len)) dead = 1;
    cJSON_free(body);
}

static int request(const char *method, cJSON *params) {
    int id = nextId++;
    cJSON *msg = cJSON_CreateObject();
    cJSON_AddNumberToObject(msg, "id", id);
    cJSON_AddStringToObject(msg, "method", method);
    cJSON_AddItemToObject(msg, "params", params);
    post(msg);
    return id;
}

static void notify(const char *method, cJSON *params) {
    cJSON *msg = cJSON_CreateObject();
    cJSON_AddStringToObject(msg, "method", method);
    cJSON_AddItemToObject(msg, "params", params);
    post(msg);
}

static cJSON *documentParam(void) {
    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(cJSON_AddObjectToObject(params, "textDocument"), "uri", uri);
    return params;
}

static void askForTokens(void) {
    tokensId = request("textDocument/semanticTokens/full", documentParam());
}

static void openDocument(void) {
    char *text = documentText();
    if (!text) return;
    cJSON *params = cJSON_CreateObject();
    cJSON *doc = cJSON_AddObjectToObject(params, "textDocument");
    cJSON_AddStringToObject(doc, "uri", uri);
    cJSON_AddStringToObject(doc, "languageId", cppFile ? "cpp" : "c");
    cJSON_AddNumberToObject(doc, "version", ++version);
    cJSON_AddStringToObject(doc, "text", text);
    free(text);
    notify("textDocument/didOpen", params);
    askForTokens();
}

static void sendChange(void) {
    char *text = documentText();
    if (!text) return;
    cJSON *params = documentParam();
    cJSON_AddNumberToObject(cJSON_GetObjectItem(params, "textDocument"), "version", ++version);
    cJSON *whole = cJSON_CreateObject(); // no "range" = this is the whole file
    cJSON_AddStringToObject(whole, "text", text);
    cJSON_AddItemToArray(cJSON_AddArrayToObject(params, "contentChanges"), whole);
    free(text);
    notify("textDocument/didChange", params);
    askForTokens();
}

static void start(void) {
    char clangd[1024], cmd[2048];
    if (!findTool("clangd.exe", clangd, sizeof(clangd))) return;
    snprintf(cmd, sizeof(cmd), "\"%s\" --log=error", clangd);
    if (!spawnServer(cmd)) return;
    running = 1;

    const char *utf8[] = { "utf-8" };
    const char *relative[] = { "relative" };
    cJSON *params = cJSON_CreateObject();
    cJSON_AddNullToObject(params, "rootUri");
    cJSON *caps = cJSON_AddObjectToObject(params, "capabilities");
    // count columns in bytes like cursorColumn does (offsetEncoding is clangd's older name for it)
    cJSON_AddItemToObject(cJSON_AddObjectToObject(caps, "general"), "positionEncodings", cJSON_CreateStringArray(utf8, 1));
    cJSON_AddItemToObject(caps, "offsetEncoding", cJSON_CreateStringArray(utf8, 1));
    cJSON *sem = cJSON_AddObjectToObject(cJSON_AddObjectToObject(caps, "textDocument"), "semanticTokens");
    cJSON_AddTrueToObject(cJSON_AddObjectToObject(sem, "requests"), "full");
    cJSON_AddItemToObject(sem, "formats", cJSON_CreateStringArray(relative, 1));
    cJSON_AddArrayToObject(sem, "tokenTypes");
    cJSON_AddArrayToObject(sem, "tokenModifiers");
    // a loose file has no build settings, so clangd uses these. same language + standard as Compile
    cJSON *flags = cJSON_AddArrayToObject(cJSON_AddObjectToObject(params, "initializationOptions"), "fallbackFlags");
    cJSON_AddItemToArray(flags, cJSON_CreateString(cppFile ? "-xc++" : "-xc"));
    cJSON_AddItemToArray(flags, cJSON_CreateString(cppFile ? "-std=c++17" : "-std=c11"));
    initId = request("initialize", params);
}

// receiving

static void gotInitialize(cJSON *result) {
    // the list of kind names. a token's "type" number is a position in this list
    cJSON *caps = cJSON_GetObjectItem(result, "capabilities");
    cJSON *legend = cJSON_GetObjectItem(cJSON_GetObjectItem(caps, "semanticTokensProvider"), "legend");
    cJSON *names = cJSON_GetObjectItem(legend, "tokenTypes");
    int n = cJSON_GetArraySize(names);
    typeNames = calloc(n > 0 ? n : 1, sizeof(char *));
    if (typeNames) {
        for (int i = 0; i < n; i++) {
            const char *s = cJSON_GetStringValue(cJSON_GetArrayItem(names, i));
            typeNames[i] = copyString(s ? s : "");
        }
        typeCount = n;
    }
    ready = 1;
    notify("initialized", cJSON_CreateObject());
    if (uri) openDocument();
}

// "data" is 5 numbers per token: line (from the last token), column (from the last
// token if it's on the same line), length, type, modifiers
static void gotTokens(cJSON *result) {
    cJSON *data = cJSON_GetObjectItem(result, "data");
    if (!cJSON_IsArray(data)) return; // an error reply: keep the old tokens
    int n = cJSON_GetArraySize(data) / 5;
    LspToken *grown = realloc(tokens, (n > 0 ? n : 1) * sizeof(LspToken));
    if (!grown) return; // out of memory: keep the old tokens
    tokens = grown;
    int line = 0, col = 0;
    cJSON *v = data ? data->child : NULL;
    for (int i = 0; i < n; i++) {
        int dLine = v->valueint; v = v->next;
        int dCol = v->valueint;  v = v->next;
        int len = v->valueint;   v = v->next;
        int type = v->valueint;  v = v->next;
        v = v->next; // modifiers: not used yet
        line += dLine;
        col = dLine ? dCol : col + dCol;
        tokens[i] = (LspToken){ line, col, len, type };
    }
    tokenCount = n;
}

static void handle(cJSON *msg) {
    cJSON *id = cJSON_GetObjectItem(msg, "id");
    cJSON *method = cJSON_GetObjectItem(msg, "method");
    if (method && id) { // clangd asking us something. answer "nothing" so it doesn't wait
        cJSON *reply = cJSON_CreateObject();
        cJSON_AddItemToObject(reply, "id", cJSON_Duplicate(id, 1));
        cJSON_AddNullToObject(reply, "result");
        post(reply);
        return;
    }
    if (!cJSON_IsNumber(id)) return; // a notification (errors, progress): not used yet
    cJSON *result = cJSON_GetObjectItem(msg, "result");
    if (id->valueint == initId) gotInitialize(result);
    else if (id->valueint == tokensId) gotTokens(result);
}

static void readMessages(void) {
    char buf[8192];
    int n;
    while ((n = readServer(buf, sizeof(buf))) > 0) {
        if (inboxLen + n > inboxCap) {
            int cap = inboxCap ? inboxCap * 2 : 65536;
            while (cap < inboxLen + n) cap *= 2;
            char *grown = realloc(inbox, cap);
            if (!grown) { // out of memory: give up on clangd
                dead = 1;
                return;
            }
            inbox = grown;
            inboxCap = cap;
        }
        memcpy(inbox + inboxLen, buf, n);
        inboxLen += n;
    }

    for (;;) {
        // the header ends at the first blank line
        int headerEnd = -1;
        for (int i = 0; i + 3 < inboxLen; i++) {
            if (inbox[i] == '\r' && inbox[i + 1] == '\n' && inbox[i + 2] == '\r' && inbox[i + 3] == '\n') {
                headerEnd = i + 4;
                break;
            }
        }
        if (headerEnd < 0) return;

        char header[256];
        int h = headerEnd < (int)sizeof(header) - 1 ? headerEnd : (int)sizeof(header) - 1;
        memcpy(header, inbox, h);
        header[h] = '\0';
        char *length = strstr(header, "Content-Length:");
        int bodyLen = length ? atoi(length + 15) : 0;
        if (inboxLen < headerEnd + bodyLen) return; // the rest of the json isn't here yet

        cJSON *msg = cJSON_ParseWithLength(inbox + headerEnd, bodyLen);
        if (msg) {
            handle(msg);
            cJSON_Delete(msg);
        }
        int used = headerEnd + bodyLen;
        memmove(inbox, inbox + used, inboxLen - used);
        inboxLen -= used;
    }
}

// what the editor calls

void lspOpen(const char *path) {
    int lang = path ? language(path) : 0;
    if (!lang) { // not C/C++: nothing for clangd to do
        lspStop();
        return;
    }
    char *newUri = makeUri(path);
    if (!newUri) return;
    if (running && (lang == 2) != cppFile) lspStop(); // -xc / -xc++ is fixed when clangd starts
    if (ready && uri) notify("textDocument/didClose", documentParam());
    free(uri);
    uri = newUri;
    cppFile = (lang == 2);
    tokenCount = 0;
    if (!running) start(); // opens the file once clangd answers initialize
    else if (ready) openDocument();
}

void lspChanged(void) {
    if (!running) return;
    changed = 1;
    changedAt = GetTime();
}

void lspUpdate(void) {
    if (!running) return;
    if (!serverAlive()) { // clangd crashed or quit: no tokens, the editor carries on
        lspStop();
        return;
    }
    readMessages();
    // send the text once typing pauses, not on every key
    if (ready && changed && GetTime() - changedAt > 0.3) {
        changed = 0;
        sendChange();
    }
    if (dead) lspStop();
}

void lspStop(void) {
    killServer();
    running = ready = dead = changed = 0;
    inboxLen = 0;
    tokenCount = 0;
    for (int i = 0; i < typeCount; i++) free(typeNames[i]);
    free(typeNames);
    typeNames = NULL;
    typeCount = 0;
    free(uri);
    uri = NULL;
}

const LspToken *lspTokens(int *count) {
    *count = tokenCount;
    return tokens;
}

const char *lspTypeName(int type) {
    return type >= 0 && type < typeCount ? typeNames[type] : "?";
}

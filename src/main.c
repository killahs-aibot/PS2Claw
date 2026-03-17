/*
 * PS2Claw - AI Assistant for PlayStation 2
 * Terminal Interface - OpenClaw CLI Experience
 * 
 * Build: make
 * Target: PlayStation 2 (MIPS R5900) via FreeMCBoot
 * 
 * Multi-Provider Support: OpenAI, Google, Anthropic, DeepSeek, xAI
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <curl/curl.h>
#include <debug.h>
#include <libkbd.h>
#include <ps2kbd.h>
#include "pad.h"
#include "keyboard.h"
#include "terminal.h"

/* Demo mode responses - OpenClaw style */
#define DEMO_RESPONSES 10
static const char *demo_responses[DEMO_RESPONSES] = {
    "I am PS2Claw, running on PlayStation 2. Your AI assistant with OpenClaw functionality on legacy hardware.",
    "PS2Claw connected. Ready to assist. Type your message or use /help for commands.",
    "OpenClaw-style interface on PS2 hardware. Multi-provider AI support: OpenAI, Google, Anthropic, DeepSeek, xAI.",
    "Running on PlayStation 2 - MIPS R5900 @ 300MHz, 32MB RAM.",
    "PS2Claw v1.6 - Terminal Mode. Neural network ready. Network stack operational.",
    "Connected to AI providers. Type freely or use commands like /help, /status, /provider.",
    "Ready. Waiting for input. Use controller or keyboard to interact.",
    "PS2Claw operational. All systems nominal. Ready for your queries.",
    "Demo mode active. In full mode, connects to OpenAI, Google, Anthropic, DeepSeek, or xAI.",
    "PS2Claw - OpenClaw on PlayStation 2. Type /help for available commands."
};

/* Max providers */
#define MAX_PROVIDERS 5
#define MAX_PROMPT 1024
#define MAX_RESPONSE 4096

/* Provider types */
typedef enum {
    AUTH_BEARER,       /* Authorization: Bearer <token> */
    AUTH_API_KEY       /* x-api-key: <token> (Anthropic) */
} AuthType;

/* Provider struct */
typedef struct {
    const char *name;           /* Display name */
    const char *id;             /* Config section ID */
    char api_key[128];          /* API key */
    char model[64];             /* Model name */
    const char *endpoint;       /* API endpoint base URL */
    AuthType auth_type;         /* Auth header format */
    int enabled;                /* Configured and ready */
} Provider;

/* Provider definitions */
static Provider providers[MAX_PROVIDERS] = {
    {
        .name = "OpenAI",
        .id = "openai",
        .api_key = {0},
        .model = "gpt-4o-mini",
        .endpoint = "https://api.openai.com/v1/chat/completions",
        .auth_type = AUTH_BEARER,
        .enabled = 0
    },
    {
        .name = "Google",
        .id = "google",
        .api_key = {0},
        .model = "gemini-1.5-flash-8b",
        .endpoint = "https://generativelanguage.googleapis.com/v1beta/models",
        .auth_type = AUTH_BEARER,
        .enabled = 0
    },
    {
        .name = "Anthropic",
        .id = "anthropic",
        .api_key = {0},
        .model = "claude-3-haiku-20240307",
        .endpoint = "https://api.anthropic.com/v1/messages",
        .auth_type = AUTH_API_KEY,
        .enabled = 0
    },
    {
        .name = "DeepSeek",
        .id = "deepseek",
        .api_key = {0},
        .model = "deepseek-chat",
        .endpoint = "https://api.deepseek.com/v1/chat/completions",
        .auth_type = AUTH_BEARER,
        .enabled = 0
    },
    {
        .name = "xAI",
        .id = "xai",
        .api_key = {0},
        .model = "grok-2-1212",
        .endpoint = "https://api.x.ai/v1/chat/completions",
        .auth_type = AUTH_BEARER,
        .enabled = 0
    }
};

static int current_provider = 0;  /* Current selected provider index */
static int num_providers = MAX_PROVIDERS;

/* Demo mode flag */
static int demo_mode = 1;
static int demo_index = 0;

/* Terminal/Keyboard state */
static int show_keyboard = 0;
static int cursor_blink = 0;
static int blink_counter = 0;
static int needs_render = 1;
static int last_render_state = -1;

/* Config file */
#define CONFIG_FILE "mc0:/PS2CLAW/config.txt"
#define MAX_CONFIG_LINE 256
#define MAX_SECTION 32

/* Forward declarations */
static void load_config(void);
static void cycle_provider(void);
static Provider* get_current_provider(void);
static int chat_request(const char *prompt, Provider *provider, char *response, size_t resp_size);
static void save_chat_log(const char *prompt, const char *response);
static void process_command(const char *cmd);
static void show_help(void);
static void show_status(void);
static void ps2_sleep(int ms);

/* Sleep function for delays */
static void ps2_sleep(int ms) {
    volatile int i, j, k;
    for (i = 0; i < ms * 100; i++) {
        for (j = 0; j < 100; j++) {
            for (k = 0; k < 10; k++) { }
        }
    }
}

/* Trim whitespace from both ends */
static void trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
}

/* Load config with INI-style sections */
static void load_config(void) {
    FILE *fp = fopen(CONFIG_FILE, "r");
    if (!fp) {
        term_printf(TERM_COLOR_INFO, "[CFG] No config file, using defaults");
        return;
    }
    
    char line[MAX_CONFIG_LINE];
    char current_section[MAX_SECTION] = {0};
    int i;
    
    while (fgets(line, sizeof(line), fp)) {
        /* Check for section header [section] */
        if (line[0] == '[') {
            char *end = strchr(line, ']');
            if (end) {
                *end = '\0';
                strcpy(current_section, line + 1);
                /* Find matching provider */
                for (i = 0; i < num_providers; i++) {
                    if (strcmp(current_section, providers[i].id) == 0) {
                        providers[i].enabled = 1;
                        break;
                    }
                }
            }
            continue;
        }
        
        /* Skip empty lines and comments */
        if (line[0] == '#' || line[0] == ';' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        
        /* Parse key=value */
        char key[MAX_CONFIG_LINE], value[MAX_CONFIG_LINE];
        char *eq = strchr(line, '=');
        if (!eq) continue;
        
        *eq = '\0';
        strcpy(key, line);
        strcpy(value, eq + 1);
        
        /* Trim whitespace */
        trim(key);
        trim(value);
        
        /* Find matching provider for this section */
        int provider_idx = -1;
        for (i = 0; i < num_providers; i++) {
            if (strcmp(current_section, providers[i].id) == 0) {
                provider_idx = i;
                break;
            }
        }
        
        if (provider_idx < 0) {
            /* Check for global settings */
            if (strcmp(key, "demo_mode") == 0) {
                demo_mode = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            } else if (strcmp(key, "default_provider") == 0) {
                for (i = 0; i < num_providers; i++) {
                    if (strcmp(value, providers[i].name) == 0 || strcmp(value, providers[i].id) == 0) {
                        current_provider = i;
                        break;
                    }
                }
            }
            continue;
        }
        
        /* Provider-specific settings */
        if (strcmp(key, "api_key") == 0) {
            strncpy(providers[provider_idx].api_key, value, sizeof(providers[provider_idx].api_key) - 1);
            term_printf(TERM_COLOR_INFO, "[CFG] [%s] API key loaded", providers[provider_idx].name);
        } else if (strcmp(key, "model") == 0) {
            strncpy(providers[provider_idx].model, value, sizeof(providers[provider_idx].model) - 1);
            term_printf(TERM_COLOR_INFO, "[CFG] [%s] Model: %s", providers[provider_idx].name, value);
        }
    }
    
    fclose(fp);
    
    /* Auto-select first enabled provider */
    for (i = 0; i < num_providers; i++) {
        if (providers[i].enabled && providers[i].api_key[0]) {
            current_provider = i;
            break;
        }
    }
}

/* Cycle to next provider */
static void cycle_provider(void) {
    int start = current_provider;
    do {
        current_provider = (current_provider + 1) % num_providers;
        if (providers[current_provider].enabled && providers[current_provider].api_key[0]) {
            break;
        }
    } while (current_provider != start);
    
    /* If no enabled provider found, stay on current */
    if (!providers[current_provider].enabled || !providers[current_provider].api_key[0]) {
        current_provider = start;
    }
}

/* Get current provider */
static Provider* get_current_provider(void) {
    return &providers[current_provider];
}

/* Chat log to USB - one file per session */
static char current_log_file[64] = {0};

static void start_chat_session(void) {
    if (current_log_file[0]) return;  /* Already started */
    
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    snprintf(current_log_file, sizeof(current_log_file), 
             "mass:/PS2CLAW/%04d%02d%02d-%02d%02d%02d.txt",
             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec);
    
    /* Create directory if needed */
    FILE *fp = fopen(current_log_file, "w");
    if (fp) {
        fprintf(fp, "=== PS2Claw Session Started ===\n\n");
        fclose(fp);
    }
}

static void save_chat_log(const char *prompt, const char *response) {
    start_chat_session();
    if (!current_log_file[0]) return;
    
    FILE *fp = fopen(current_log_file, "a");
    if (!fp) return;
    
    time_t now = time(NULL);
    char *ts = ctime(&now);
    ts[strlen(ts)-1] = '\0';
    
    fprintf(fp, "[%s]\n", ts);
    fprintf(fp, "YOU: %s\n", prompt);
    fprintf(fp, "PS2CLAW: %s\n\n", response);
    fclose(fp);
}

/* Response buffer */
typedef struct {
    char *data;
    size_t size;
} ResponseBuffer;

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    ResponseBuffer *buf = (ResponseBuffer *)userp;
    
    char *ptr = realloc(buf->data, buf->size + realsize + 1);
    if (!ptr) return 0;
    
    buf->data = ptr;
    memcpy(&buf->data[buf->size], contents, realsize);
    buf->size += realsize;
    buf->data[buf->size] = 0;
    
    return realsize;
}

/* Escape JSON string */
static void escape_json(const char *src, char *dst, size_t dst_size) {
    char *d = dst;
    const char *s = src;
    size_t remain = dst_size - 1;
    
    while (*s && remain > 1) {
        if (*s == '"' || *s == '\\') {
            if (remain < 3) break;
            *d++ = '\\';
            *d++ = *s;
            remain -= 2;
        } else if (*s == '\n') {
            if (remain < 3) break;
            *d++ = '\\';
            *d++ = 'n';
            remain -= 2;
        } else if (*s == '\t') {
            if (remain < 3) break;
            *d++ = '\\';
            *d++ = 't';
            remain -= 2;
        } else {
            *d++ = *s;
            remain--;
        }
        s++;
    }
    *d = '\0';
}

/* Extract content from OpenAI/DeepSeek/xAI style JSON response */
static char* extract_content_openai(const char *json, char *content, size_t content_size) {
    const char *p = strstr(json, "\"content\":\"");
    if (!p) p = strstr(json, "\"content\": \"");
    if (!p) return NULL;
    
    p = strchr(p, ':');
    if (!p) return NULL;
    p++;
    while (*p == ' ' || *p == '"') p++;
    
    char *end = strchr(p, '"');
    if (!end) return NULL;
    
    size_t len = end - p;
    if (len > content_size - 1) len = content_size - 1;
    
    strncpy(content, p, len);
    content[len] = 0;
    
    /* Handle escaped characters */
    char *dst = content;
    char *src = content;
    while (*src) {
        if (*src == '\\' && *(src+1) == 'n') {
            *dst++ = '\n';
            src += 2;
        } else if (*src == '\\' && *(src+1) == 't') {
            *dst++ = '\t';
            src += 2;
        } else if (*src == '\\' && *(src+1) == '"') {
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = 0;
    
    return content;
}

/* Extract content from Anthropic style JSON response */
static char* extract_content_anthropic(const char *json, char *content, size_t content_size) {
    const char *p = strstr(json, "\"text\":\"");
    if (!p) p = strstr(json, "\"text\": \"");
    if (!p) return NULL;
    
    p = strchr(p, ':');
    if (!p) return NULL;
    p++;
    while (*p == ' ' || *p == '"') p++;
    
    char *end = strchr(p, '"');
    if (!end) return NULL;
    
    size_t len = end - p;
    if (len > content_size - 1) len = content_size - 1;
    
    strncpy(content, p, len);
    content[len] = 0;
    
    return content;
}

/* Extract content from Google style JSON response */
static char* extract_content_google(const char *json, char *content, size_t content_size) {
    const char *p = strstr(json, "\"text\":\"");
    if (!p) p = strstr(json, "\"text\": \"");
    if (!p) return NULL;
    
    p = strchr(p, ':');
    if (!p) return NULL;
    p++;
    while (*p == ' ' || *p == '"') p++;
    
    char *end = strchr(p, '"');
    if (!end) return NULL;
    
    size_t len = end - p;
    if (len > content_size - 1) len = content_size - 1;
    
    strncpy(content, p, len);
    content[len] = 0;
    
    return content;
}

/* Send chat request to LLM API - handles multiple providers */
int chat_request(const char *prompt, Provider *provider, char *response, size_t resp_size) {
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    ResponseBuffer buf;
    char *json_payload;
    long http_code;
    char content[MAX_RESPONSE];
    char escaped_prompt[MAX_PROMPT * 2 + 1];
    
    buf.data = malloc(1);
    if (!buf.data) return -1;
    buf.size = 0;
    
    /* Build JSON payload - format varies by provider */
    json_payload = malloc(512 + strlen(prompt) * 2);
    if (!json_payload) {
        free(buf.data);
        return -1;
    }
    
    /* Escape the prompt */
    escape_json(prompt, escaped_prompt, sizeof(escaped_prompt));
    
    if (strcmp(provider->id, "anthropic") == 0) {
        snprintf(json_payload, 512 + strlen(escaped_prompt) * 2,
            "{\"model\":\"%s\",\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],\"max_tokens\":256}",
            provider->model, escaped_prompt);
    } else if (strcmp(provider->id, "google") == 0) {
        snprintf(json_payload, 512 + strlen(escaped_prompt) * 2,
            "{\"contents\":[{\"parts\":[{\"text\":\"%s\"}]}],\"generationConfig\":{\"maxOutputTokens\":256}}",
            escaped_prompt);
    } else {
        snprintf(json_payload, 512 + strlen(escaped_prompt) * 2,
            "{\"model\":\"%s\",\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],\"max_tokens\":256}",
            provider->model, escaped_prompt);
    }
    
    curl = curl_easy_init();
    if (!curl) {
        free(buf.data);
        free(json_payload);
        return -1;
    }
    
    /* Set headers */
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    if (provider->auth_type == AUTH_API_KEY) {
        char api_key_header[256];
        snprintf(api_key_header, sizeof(api_key_header), "x-api-key: %s", provider->api_key);
        headers = curl_slist_append(headers, api_key_header);
        headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
    } else {
        char auth_header[256];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", provider->api_key);
        headers = curl_slist_append(headers, auth_header);
    }
    
    /* Build endpoint URL */
    char url[512];
    if (strcmp(provider->id, "google") == 0) {
        snprintf(url, sizeof(url), "%s/%s:generateContent?key=%s", 
                 provider->endpoint, provider->model, provider->api_key);
    } else {
        strncpy(url, provider->endpoint, sizeof(url) - 1);
        url[sizeof(url) - 1] = 0;
    }
    
    /* Configure curl */
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    term_printf(TERM_COLOR_INFO, "[NET] Connecting to %s...", provider->name);
    
    /* Perform request */
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        term_printf(TERM_COLOR_ERROR, "[ERR] CURL: %s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        free(buf.data);
        free(json_payload);
        return -1;
    }
    
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    
    if (http_code != 200) {
        term_printf(TERM_COLOR_ERROR, "[ERR] HTTP %ld", http_code);
        if (buf.size > 0 && buf.data) {
            buf.data[buf.size < 256 ? buf.size : 256] = 0;
            term_printf(TERM_COLOR_ERROR, "[ERR] Response: %s", buf.data);
        }
        free(buf.data);
        free(json_payload);
        return -1;
    }
    
    /* Extract content based on provider */
    char *extracted = NULL;
    if (strcmp(provider->id, "anthropic") == 0) {
        extracted = extract_content_anthropic(buf.data, content, sizeof(content));
    } else if (strcmp(provider->id, "google") == 0) {
        extracted = extract_content_google(buf.data, content, sizeof(content));
    } else {
        extracted = extract_content_openai(buf.data, content, sizeof(content));
    }
    
    if (extracted) {
        strncpy(response, extracted, resp_size - 1);
        response[resp_size - 1] = 0;
    } else {
        term_printf(TERM_COLOR_ERROR, "[ERR] Parse failed");
        free(buf.data);
        free(json_payload);
        return -1;
    }
    
    free(buf.data);
    free(json_payload);
    return 0;
}

/* Show help */
static void show_help(void) {
    term_printf(TERM_COLOR_HEADER, "+----------------------------------------+");
    term_printf(TERM_COLOR_HEADER, "| PS2Claw Commands                       |");
    term_printf(TERM_COLOR_HEADER, "+----------------------------------------+");
    term_printf(TERM_COLOR_PROMPT, "/help     - Show this help message");
    term_printf(TERM_COLOR_PROMPT, "/clear    - Clear terminal screen");
    term_printf(TERM_COLOR_PROMPT, "/status   - Show connection status");
    term_printf(TERM_COLOR_PROMPT, "/model    - Show current model");
    term_printf(TERM_COLOR_PROMPT, "/provider - Cycle to next provider");
    term_printf(TERM_COLOR_PROMPT, "/demo     - Toggle demo mode");
    term_printf(TERM_COLOR_PROMPT, "/history  - Show command history");
    term_printf(TERM_COLOR_PROMPT, "/quit     - Exit PS2Claw");
    term_printf(TERM_COLOR_PROMPT, "");
    term_printf(TERM_COLOR_PROMPT, "Just type to chat with AI!");
    term_printf(TERM_COLOR_HEADER, "+----------------------------------------+");
}

/* Show status */
static void show_status(void) {
    Provider *cp = get_current_provider();
    
    term_printf(TERM_COLOR_HEADER, "+----------------------------------------+");
    term_printf(TERM_COLOR_HEADER, "| PS2Claw Status                         |");
    term_printf(TERM_COLOR_HEADER, "+----------------------------------------+");
    term_printf(TERM_COLOR_INFO, "Provider: %s", cp->name);
    term_printf(TERM_COLOR_INFO, "Model: %s", cp->model);
    term_printf(TERM_COLOR_INFO, "Mode: %s", demo_mode ? "DEMO" : "ONLINE");
    term_printf(TERM_COLOR_INFO, "Available Providers:");
    
    for (int i = 0; i < num_providers; i++) {
        if (providers[i].enabled && providers[i].api_key[0]) {
            if (i == current_provider) {
                term_printf(TERM_COLOR_PROMPT, "  * %s (active)", providers[i].name);
            } else {
                term_printf(TERM_COLOR_DEFAULT, "    %s", providers[i].name);
            }
        }
    }
    
    term_printf(TERM_COLOR_HEADER, "+----------------------------------------+");
}

/* Process command */
static void process_command(const char *cmd) {
    if (!cmd || cmd[0] != '/') return;
    
    if (strcmp(cmd, "/help") == 0) {
        show_help();
    } else if (strcmp(cmd, "/clear") == 0) {
        term_clear();
    } else if (strcmp(cmd, "/status") == 0) {
        show_status();
    } else if (strcmp(cmd, "/model") == 0) {
        Provider *cp = get_current_provider();
        term_printf(TERM_COLOR_INFO, "Current model: %s", cp->model);
    } else if (strcmp(cmd, "/provider") == 0) {
        cycle_provider();
        Provider *cp = get_current_provider();
        term_printf(TERM_COLOR_INFO, "Switched to: %s", cp->name);
    } else if (strcmp(cmd, "/demo") == 0) {
        demo_mode = !demo_mode;
        term_printf(TERM_COLOR_INFO, "Demo mode: %s", demo_mode ? "ON" : "OFF");
    } else if (strcmp(cmd, "/history") == 0) {
        /* Show history from keyboard module */
        term_printf(TERM_COLOR_INFO, "Use Up/Down to cycle command history");
    } else if (strcmp(cmd, "/quit") == 0 || strcmp(cmd, "/exit") == 0) {
        term_printf(TERM_COLOR_INFO, "Session terminated. Goodbye!");
        ps2_sleep(2000);
        /* In a real implementation, this would exit */
    } else {
        term_printf(TERM_COLOR_ERROR, "Unknown command: %s", cmd);
        term_printf(TERM_COLOR_INFO, "Type /help for available commands");
    }
}

/* Handle controller input */
static void handle_input(void) {
    /* Poll controller */
    pad_poll();
    
    /* Handle keyboard if visible */
    if (show_keyboard) {
        kb_handle_input();
        kb_update();
        
        /* Check for enter */
        if (kb_enter_pressed()) {
            const char *text = kb_get_text();
            if (text && strlen(text) > 0) {
                /* Print user's input */
                term_printf(TERM_COLOR_PROMPT, "> %s", text);
                
                /* Check for commands */
                if (text[0] == '/') {
                    process_command(text);
                } else {
                    /* Chat with AI */
                    if (demo_mode) {
                        term_printf(TERM_COLOR_OUTPUT, "%s", demo_responses[demo_index]);
                        save_chat_log(text, demo_responses[demo_index]);
                        demo_index = (demo_index + 1) % DEMO_RESPONSES;
                    } else {
                        Provider *cp = get_current_provider();
                        if (!cp->enabled || !cp->api_key[0]) {
                            term_printf(TERM_COLOR_ERROR, "No provider configured. Type /demo for offline mode.");
                        } else {
                            term_printf(TERM_COLOR_INFO, "[ Processing... ]");
                            char response[MAX_RESPONSE];
                            if (chat_request(text, cp, response, sizeof(response)) == 0) {
                                term_printf(TERM_COLOR_OUTPUT, "%s", response);
                                save_chat_log(text, response);
                            } else {
                                term_printf(TERM_COLOR_ERROR, "Request failed. Try /demo for offline mode.");
                            }
                        }
                    }
                }
                
                /* Add to history */
                kb_add_history(text);
                kb_clear();
            }
            term_invalidate();
        }
        
        /* Check for escape */
        if (kb_escape_pressed()) {
            show_keyboard = 0;
            kb_hide();
            needs_render = 1;
            term_invalidate();
        }
        
        return;
    }
    
    /* Global controls when keyboard is hidden */
    
    /* X button - show keyboard */
    if (pad_button_just_pressed(PAD_CROSS)) {
        show_keyboard = 1;
        kb_show();
        kb_clear();
        needs_render = 1;
        term_invalidate();
        return;
    }
    
    /* Triangle - toggle demo mode */
    if (pad_button_just_pressed(PAD_TRIANGLE)) {
        demo_mode = !demo_mode;
        term_printf(TERM_COLOR_INFO, "Demo mode: %s", demo_mode ? "ON" : "OFF");
        needs_render = 1;
        term_invalidate();
        return;
    }
    
    /* Square - show help */
    if (pad_button_just_pressed(PAD_SQUARE)) {
        show_help();
        needs_render = 1;
        term_invalidate();
        return;
    }
    
    /* R1 - cycle provider */
    if (pad_button_just_pressed(PAD_R1)) {
        cycle_provider();
        Provider *cp = get_current_provider();
        term_printf(TERM_COLOR_INFO, "Provider: %s", cp->name);
        needs_render = 1;
        term_invalidate();
        return;
    }
    
    /* L1 - show status */
    if (pad_button_just_pressed(PAD_L1)) {
        show_status();
        needs_render = 1;
        term_invalidate();
        return;
    }
    
    /* Start - show status */
    if (pad_button_just_pressed(PAD_START)) {
        show_status();
        needs_render = 1;
        term_invalidate();
        return;
    }
}

/* Render everything - only update when state changes */
static void render(void) {
    int current_state = show_keyboard ? 1 : 0;
    
    /* Skip render if nothing changed (stops scrolling) */
    if (current_state == last_render_state && !needs_render) {
        return;
    }
    
    last_render_state = current_state;
    needs_render = 0;
    
    /* Clear screen */
    scr_printf("\x1b[2J");
    scr_printf("\x1b[32m");  /* Green text */
    
    /* Get provider */
    Provider *cp = get_current_provider();
    
    if (!show_keyboard) {
        /* Simple terminal view - like Linux console - TIGHT spacing */
        
        scr_printf("\n\n");  /* Push down for TV overscan */
        scr_printf("PS2Claw v1.6 on PlayStation 2\n");
        scr_printf("%s %s", cp->name, cp->model);
        if (demo_mode) scr_printf(" [DEMO]");
        scr_printf("\n");
        scr_printf("Type /help for commands\n");
        scr_printf("Press X or START to type\n");
        
        /* Prompt at bottom */
        blink_counter++;
        if (blink_counter > 30) {
            blink_counter = 0;
            cursor_blink = !cursor_blink;
        }
        
        scr_printf("$ ");
        if (cursor_blink) scr_printf("█");
        else scr_printf(" ");
    } else {
        /* Keyboard view - simple */
        scr_printf("\n");
        scr_printf("PS2Claw v1.6 on PlayStation 2\n\n");
        
        /* Current input */
        scr_printf("> %s", kb_get_text());
        
        /* Cursor blink */
        blink_counter++;
        if (blink_counter > 30) {
            blink_counter = 0;
            cursor_blink = !cursor_blink;
        }
        
        if (cursor_blink) {
            scr_printf("_");
        }
        
        scr_printf("\n\n");
        
        /* Draw keyboard */
        kb_render();
        
        scr_printf("\n$ ");
    }
}

/* Main program */
int main(int argc, char *argv[]) {
    char response[MAX_RESPONSE];
    
    /* Initialize debug screen */
    init_scr();
    scr_setCursor(0);
    
    /* Initialize pad controller */
    int pad_result = pad_init();
    if (pad_result < 0) {
        scr_printf("[PAD] Warning: No controller detected, using keyboard\n");
    }
    
    /* Initialize keyboard (fallback) */
    PS2KbdInit();
    
    /* Initialize terminal and keyboard */
    term_init();
    kb_init();
    
    /* Boot sequence */
    scr_printf("\x1b[2J");  /* Clear screen */
    scr_printf("\n");  /* Push down for overscan */
    scr_printf("PS2Claw v1.6 - Terminal Mode\n");
    ps2_sleep(300);
    scr_printf("OpenClaw on PlayStation 2\n");
    ps2_sleep(300);
    scr_printf("MIPS R5900 @ 300MHz, 32MB RAM\n");
    ps2_sleep(300);
    scr_printf("AI: ENABLED\n");
    scr_printf("Providers: OpenAI, Google, Anthropic, DeepSeek, xAI\n");
    ps2_sleep(500);
    
    /* Load configuration */
    load_config();
    
    /* Show initial status */
    Provider *cp = get_current_provider();
    if (cp->enabled && cp->api_key[0]) {
        scr_printf("Provider: %s\n", cp->name);
        scr_printf("Model: %s\n", cp->model);
    } else {
        scr_printf("No provider configured (demo mode)\n");
    }
    
    scr_printf("%s mode\n", demo_mode ? "Demo" : "Online");
    ps2_sleep(1000);
    
    /* Main loop */
    while (1) {
        /* Handle input */
        handle_input();
        
        /* Render */
        render();
        
        /* Small delay */
        ps2_sleep(50);
    }
    
    /* Cleanup */
    pad_shutdown();
    PS2KbdClose();
    
    return 0;
}
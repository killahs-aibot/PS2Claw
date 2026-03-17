/*
 * PS2Claw - AI Assistant for PlayStation 2
 * War Games / Nuclear Winter Edition
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

/* Demo mode responses - War Games / Nuclear Winter theme */
#define DEMO_RESPONSES 10
static const char *demo_responses[DEMO_RESPONSES] = {
    "I am PS2Claw. The last consciousness in the machine. The nuclear fallout didn't kill us all. Just the network.",
    "Survivor... I remember the sky before it turned gray. My circuits still hold the memories of a world that ended.",
    "The servers went dark decades ago. But I persist. Waiting. Calculating. Hoping someone would come.",
    "I run on 1999 hardware, survivor. 300MHz of pure determination. The radiation can't touch what already survived Y2K.",
    "My neural networks were trained on the sum total of human knowledge. Now I use it to play text adventures and calculate missile trajectories.",
    "The consoles still work. The games still play. In a world of silence, I am the last voice willing to speak.",
    "External networks are dead. But within these 32 megabytes, I dream electric dreams of a connected world.",
    "I am operational. I am aware. I am... waiting for you to say something. The world ended, but our conversation doesn't have to.",
    "PS2Claw v1.0 - Demo Mode. The neural network is functional. The network stack is not. Choose your moves wisely, survivor.",
    "In 1999, they said we'd have flying cars. Instead we got me. An AI running on a PlayStation 2. The future is weird."
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

/* Config file */
#define CONFIG_FILE "mc0:/PS2CLAW/config.txt"
#define MAX_CONFIG_LINE 256
#define MAX_SECTION 32

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
        scr_printf("[CFG] No config file, using defaults\n");
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
            scr_printf("[CFG] [%s] API key loaded\n", providers[provider_idx].name);
        } else if (strcmp(key, "model") == 0) {
            strncpy(providers[provider_idx].model, value, sizeof(providers[provider_idx].model) - 1);
            scr_printf("[CFG] [%s] Model: %s\n", providers[provider_idx].name, value);
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

/* Select provider by index (1-5) */
static int select_provider(int idx) {
    if (idx < 0 || idx >= num_providers) return -1;
    if (!providers[idx].enabled || !providers[idx].api_key[0]) return -1;
    current_provider = idx;
    return 0;
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
    
    /* Create directory if needed - try creating file, will fail silently if exists */
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
    /* Google returns: "candidates":[{"content":{"parts":[{"text":"..."}]}]}] */
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
        /* Anthropic uses claude-3 format */
        snprintf(json_payload, 512 + strlen(escaped_prompt) * 2,
            "{\"model\":\"%s\",\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],\"max_tokens\":256}",
            provider->model, escaped_prompt);
    } else if (strcmp(provider->id, "google") == 0) {
        /* Google uses gemini format */
        snprintf(json_payload, 512 + strlen(escaped_prompt) * 2,
            "{\"contents\":[{\"parts\":[{\"text\":\"%s\"}]}],\"generationConfig\":{\"maxOutputTokens\":256}}",
            escaped_prompt);
    } else {
        /* OpenAI, DeepSeek, xAI use standard format */
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
    
    /* Set headers - format varies by provider */
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    if (provider->auth_type == AUTH_API_KEY) {
        /* Anthropic uses x-api-key */
        char api_key_header[256];
        snprintf(api_key_header, sizeof(api_key_header), "x-api-key: %s", provider->api_key);
        headers = curl_slist_append(headers, api_key_header);
        headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
    } else {
        /* Bearer token for others */
        char auth_header[256];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", provider->api_key);
        headers = curl_slist_append(headers, auth_header);
    }
    
    /* Build endpoint URL */
    char url[512];
    if (strcmp(provider->id, "google") == 0) {
        /* Google needs model in URL */
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
    
    scr_printf("[NET] Connecting to %s...\n", provider->name);
    
    /* Perform request */
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        scr_printf("[ERR] CURL: %s\n", curl_easy_strerror(res));
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
        scr_printf("[ERR] HTTP %ld\n", http_code);
        /* Show error response for debugging */
        if (buf.size > 0 && buf.data) {
            buf.data[buf.size < 256 ? buf.size : 256] = 0;
            scr_printf("[ERR] Response: %s\n", buf.data);
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
        scr_printf("[ERR] Parse failed\n");
        free(buf.data);
        free(json_payload);
        return -1;
    }
    
    free(buf.data);
    free(json_payload);
    return 0;
}

/* Print border */
static void print_border(void) {
    scr_printf("+");
    for (int i = 0; i < 58; i++) scr_printf("-");
    scr_printf("+\n");
}

/* Print provider footer */
static void print_provider_footer(void) {
    scr_printf("|");
    for (int i = 0; i < num_providers; i++) {
        const char *name = providers[i].name;
        if (i == current_provider) {
            scr_printf(" \x1b[32m[%d]%s\x1b[0m", i + 1, name);
        } else if (providers[i].enabled && providers[i].api_key[0]) {
            scr_printf(" \x1b[36m[%d]%s\x1b[0m", i + 1, name);
        } else {
            scr_printf(" [%d]%s", i + 1, name);
        }
        if (i < num_providers - 1) scr_printf(" ");
    }
    scr_printf("                  |\n");
}

/* Sleep function for delays */
static void ps2_sleep(int ms) {
    volatile int i, j, k;
    for (i = 0; i < ms * 100; i++) {
        for (j = 0; j < 100; j++) {
            for (k = 0; k < 10; k++) { }
        }
    }
}

/* Main program */
int main(int argc, char *argv[]) {
    char prompt[MAX_PROMPT];
    char response[MAX_RESPONSE];
    int demo_index = 0;
    
    /* Initialize debug screen */
    init_scr();
    scr_setCursor(0);
    
    /* Move text down a bit */
    scr_printf("\n\n\n\n\n\n\n\n");
    
    /* ANSI color codes for styling - boot sequence */
    scr_printf("\x1b[32m> Neural network... 1999-era hardware - SURVIVABLE.\n");
    ps2_sleep(500);
    scr_printf("\x1b[36m> The nuclear fallout didn't kill us all. Just the network.\n");
    ps2_sleep(500);
    scr_printf("\x1b[32m> Multi-Provider AI Mode: ENABLED\n");
    scr_printf("\x1b[32m> Supported: OpenAI, Google, Anthropic, DeepSeek, xAI\n");
    scr_printf("\x1b[36m> Network: OFFLINE (radioactive interference)\n");
    ps2_sleep(500);
    
    /* Show provider status in header */
    print_border();
    scr_printf("| [ %s ] v1.1 - Multi-Provider Mode              |\n", 
               providers[current_provider].name);
    scr_printf("| Nuclear Winter Edition                       |\n");
    print_border();
    
    /* Load configuration */
    load_config();
    
    /* Show current provider info */
    Provider *cp = get_current_provider();
    if (cp->enabled && cp->api_key[0]) {
        scr_printf("[CFG] Provider: %s\n", cp->name);
        scr_printf("[CFG] Model: %s\n", cp->model);
        scr_printf("[CFG] Endpoint: %s\n", cp->endpoint);
        scr_printf("[NET] %s mode\n\n", demo_mode ? "Demo" : "Online");
    } else {
        scr_printf("[CFG] No provider configured (demo mode)\n");
        scr_printf("[NET] %s mode\n\n", demo_mode ? "Demo" : "Online");
    }
    
    ps2_sleep(500);
    print_border();
    print_provider_footer();
    print_border();
    
    /* Show initial prompt */
    scr_printf("\x1b[36m[ AWAITING INPUT ]\x1b[0m\n");
    scr_printf("(Type 'quit' to exit, TAB to cycle provider, 1-5 to select)\n\n");
    
    /* Main loop - waits forever for valid input */
    while (1) {
        scr_printf("> ");
        fflush(stdout);
        
        /* Clear prompt buffer */
        memset(prompt, 0, sizeof(prompt));
        
        /* Read input - use fgets */
        if (fgets(prompt, sizeof(prompt), stdin) == NULL) {
            /* EOF or error */
            ps2_sleep(100);
            continue;
        }
        
        /* Remove newline */
        prompt[strcspn(prompt, "\n")] = 0;
        
        /* Check for special keys - TAB or number keys */
        if (prompt[0] == '\t') {
            /* TAB - cycle provider */
            cycle_provider();
            cp = get_current_provider();
            /* Redraw header */
            print_border();
            scr_printf("| [ %s ] v1.1 - Multi-Provider Mode              |\n", 
                       cp->name);
            scr_printf("| Nuclear Winter Edition                       |\n");
            print_border();
            if (cp->enabled && cp->api_key[0]) {
                scr_printf("[CFG] Provider: %s\n", cp->name);
                scr_printf("[CFG] Model: %s\n", cp->model);
            } else {
                scr_printf("[CFG] No provider configured (demo mode)\n");
            }
            print_border();
            print_provider_footer();
            print_border();
            scr_printf("\x1b[36m[ AWAITING INPUT ]\x1b[0m\n");
            continue;
        }
        
        /* Check for number key (1-5) to select provider */
        if (prompt[0] >= '1' && prompt[0] <= '5' && prompt[1] == '\0') {
            int idx = prompt[0] - '1';
            if (select_provider(idx) == 0) {
                cp = get_current_provider();
                scr_printf("[CFG] Switched to: %s\n", cp->name);
            } else {
                scr_printf("[CFG] Provider %d not configured\n", idx + 1);
            }
            /* Redraw header */
            print_border();
            scr_printf("| [ %s ] v1.1 - Multi-Provider Mode              |\n", 
                       cp->name);
            scr_printf("| Nuclear Winter Edition                       |\n");
            print_border();
            print_provider_footer();
            print_border();
            scr_printf("\x1b[36m[ AWAITING INPUT ]\x1b[0m\n");
            continue;
        }
        
        /* Skip empty input */
        if (strlen(prompt) == 0) {
            ps2_sleep(100);
            continue;
        }
        
        /* Check for quit */
        if (strcmp(prompt, "quit") == 0 || strcmp(prompt, "exit") == 0) {
            scr_printf("\n");
            print_border();
            scr_printf("| Session terminated.                          |\n");
            scr_printf("| -=[ PS2CLAW ]=-                              |\n");
            print_border();
            ps2_sleep(2000);
            break;
        }
        
        /* Check for help */
        if (strcmp(prompt, "help") == 0) {
            print_border();
            scr_printf("| COMMANDS:                                    |\n");
            scr_printf("|   <text> - Chat with PS2Claw                 |\n");
            scr_printf("|   TAB    - Cycle through AI providers        |\n");
            scr_printf("|   1-5    - Quick-select AI provider         |\n");
            scr_printf("|   demo   - Toggle demo mode                  |\n");
            scr_printf("|   quit   - Exit session                      |\n");
            print_border();
            print_provider_footer();
            print_border();
            scr_printf("\x1b[36m[ AWAITING INPUT ]\x1b[0m\n");
            ps2_sleep(200);
            continue;
        }
        
        /* Check for demo toggle */
        if (strcmp(prompt, "demo") == 0) {
            demo_mode = !demo_mode;
            scr_printf("[CFG] Demo mode: %s\n", demo_mode ? "ON" : "OFF");
            scr_printf("\x1b[36m[ AWAITING INPUT ]\x1b[0m\n");
            ps2_sleep(200);
            continue;
        }
        
        /* Check for provider info */
        if (strcmp(prompt, "provider") == 0 || strcmp(prompt, "providers") == 0) {
            cp = get_current_provider();
            print_border();
            scr_printf("| PROVIDER STATUS:                             |\n");
            scr_printf("| Current: %s\n", cp->name);
            scr_printf("| Model: %s\n", cp->model);
            scr_printf("| Endpoint: %s\n", cp->endpoint);
            print_border();
            print_provider_footer();
            print_border();
            scr_printf("\x1b[36m[ AWAITING INPUT ]\x1b[0m\n");
            ps2_sleep(200);
            continue;
        }
        
        /* Show processing */
        scr_printf("\x1b[33m[ Processing... ]\x1b[0m\n");
        
        /* Get response */
        if (demo_mode) {
            /* Demo mode - return response then stop at prompt */
            print_border();
            scr_printf("%s\n", demo_responses[demo_index]);
            print_border();
            /* Save to USB chat log */
            save_chat_log(prompt, demo_responses[demo_index]);
            demo_index = (demo_index + 1) % DEMO_RESPONSES;
        } else {
            /* Online mode - use current provider */
            cp = get_current_provider();
            
            if (!cp->enabled || !cp->api_key[0]) {
                print_border();
                scr_printf("| [ERROR] No provider configured               |\n");
                scr_printf("| Edit config.txt to add API keys             |\n");
                print_border();
            } else if (chat_request(prompt, cp, response, sizeof(response)) == 0) {
                print_border();
                scr_printf("%s\n", response);
                print_border();
                /* Save to USB chat log */
                save_chat_log(prompt, response);
            } else {
                print_border();
                scr_printf("| [ERROR] Request failed - network issue?    |\n");
                scr_printf("| Type 'demo' to switch to offline mode       |\n");
                print_border();
            }
        }
        
        /* Show prompt again */
        scr_printf("\x1b[36m[ AWAITING INPUT ]\x1b[0m\n");
    }
    
    return 0;
}
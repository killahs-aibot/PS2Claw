/*
 * PS2Claw - AI Assistant for PlayStation 2
 * War Games / Nuclear Winter Edition
 * 
 * Build: make
 * Target: PlayStation 2 (MIPS R5900) via FreeMCBoot
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

/* API Configuration */
#define API_URL "https://openrouter.ai/api/v1/chat/completions"
#define DEFAULT_MODEL "google/gemini-3.1-flash-lite-preview"
#define MAX_PROMPT 1024
#define MAX_RESPONSE 4096

/* Demo mode flag */
static int demo_mode = 1;

/* Config file */
#define CONFIG_FILE "mc0:/PS2CLAW/config.txt"
#define MAX_CONFIG_LINE 256

/* Parse config value from line "key=value" */
static void parse_config_line(char *line, char *key, char *value) {
    char *eq = strchr(line, '=');
    if (eq) {
        *eq = '\0';
        strcpy(key, line);
        strcpy(value, eq + 1);
        /* Trim trailing newline */
        size_t len = strlen(value);
        if (len > 0 && value[len-1] == '\n') value[len-1] = '\0';
    }
}

/* Load config from file */
static void load_config(char *api_key, size_t api_len, char *model, size_t model_len) {
    FILE *fp = fopen(CONFIG_FILE, "r");
    if (!fp) {
        scr_printf("[CFG] No config file, using defaults\n");
        return;
    }
    
    char line[MAX_CONFIG_LINE];
    while (fgets(line, sizeof(line), fp)) {
        char key[MAX_CONFIG_LINE], value[MAX_CONFIG_LINE];
        parse_config_line(line, key, value);
        
        if (strcmp(key, "api_key") == 0) {
            strncpy(api_key, value, api_len - 1);
            api_key[api_len - 1] = '\0';
            scr_printf("[CFG] API key loaded\n");
        } else if (strcmp(key, "model") == 0) {
            strncpy(model, value, model_len - 1);
            model[model_len - 1] = '\0';
            scr_printf("[CFG] Model: %s\n", model);
        } else if (strcmp(key, "demo_mode") == 0) {
            demo_mode = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
        }
    }
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

/* Extract content from JSON response */
char *extract_content(const char *json) {
    static char content[MAX_RESPONSE];
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
    if (len > MAX_RESPONSE - 1) len = MAX_RESPONSE - 1;
    
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

/* Send chat request to LLM API */
int chat_request(const char *prompt, const char *api_key, const char *model, char *response, size_t resp_size) {
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    ResponseBuffer buf;
    char *json_payload;
    long http_code;
    char *content;
    
    buf.data = malloc(1);
    if (!buf.data) return -1;
    buf.size = 0;
    
    /* Build JSON payload */
    json_payload = malloc(512 + strlen(prompt) * 2);
    if (!json_payload) {
        free(buf.data);
        return -1;
    }
    
    /* Simple escape for prompt */
    char escaped_prompt[MAX_PROMPT * 2];
    char *ep = escaped_prompt;
    const char *p = prompt;
    while (*p) {
        if (*p == '"' || *p == '\\') *ep++ = '\\';
        *ep++ = *p++;
    }
    *ep = 0;
    
    snprintf(json_payload, 512 + strlen(escaped_prompt) * 2,
        "{\"model\":\"%s\",\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],\"max_tokens\":256}",
        model, escaped_prompt);
    
    curl = curl_easy_init();
    if (!curl) {
        free(buf.data);
        free(json_payload);
        return -1;
    }
    
    /* Set headers */
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    char auth_header[256];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "HTTP-Referer: https://ps2claw.local");
    headers = curl_slist_append(headers, "X-Title: PS2Claw");
    
    /* Configure curl */
    curl_easy_setopt(curl, CURLOPT_URL, API_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    scr_printf("[NET] Connecting...\n");
    
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
        free(buf.data);
        free(json_payload);
        return -1;
    }
    
    /* Extract content */
    content = extract_content(buf.data);
    if (content) {
        strncpy(response, content, resp_size - 1);
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
void print_border(void) {
    scr_printf("+");
    for (int i = 0; i < 58; i++) scr_printf("-");
    scr_printf("+\n");
}

/* Sleep function for delays */
void ps2_sleep(int ms) {
    volatile int i, j, k;
    for (i = 0; i < ms * 100; i++) {
        for (j = 0; j < 100; j++) {
            for (k = 0; k < 10; k++) { }
        }
    }
}

/* Main program */
int main(int argc, char *argv[]) {
    char *api_key;
    char *model;
    char prompt[MAX_PROMPT];
    char response[MAX_RESPONSE];
    int demo_index = 0;
    int first_run = 1;
    
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
    scr_printf("\x1b[32m> Demo mode: ENABLED\n");
    scr_printf("\x1b[32m> Network: OFFLINE (radioactive interference)\n");
    ps2_sleep(500);
    scr_printf("\x1b[36m[ PS2CLAW ONLINE - DEMO MODE ]\n");
    scr_printf("\x1b[0m");
    ps2_sleep(800);
    
    print_border();
    scr_printf("| PS2CLAW v1.0 - Demo Mode                     |\n");
    scr_printf("| Nuclear Winter Edition                       |\n");
    print_border();
    
    /* Get model from env or use default */
    model = getenv("MODEL");
    if (!model) model = DEFAULT_MODEL;
    
    /* Load API key from config file */
    char config_api_key[128] = {0};
    char config_model[64] = {0};
    load_config(config_api_key, sizeof(config_api_key), config_model, sizeof(config_model));
    
    /* Use config API key if loaded, otherwise env */
    if (config_api_key[0]) {
        api_key = config_api_key;
    } else {
        api_key = getenv("API_KEY");
    }
    
    /* Use config model if loaded */
    if (config_model[0]) {
        model = config_model;
    }
    
    scr_printf("[CFG] Model: %s\n", model);
    if (api_key) {
        scr_printf("[CFG] API key: loaded\n");
    } else {
        scr_printf("[CFG] API key: missing (demo mode)\n");
    }
    scr_printf("[NET] %s mode\n\n", demo_mode ? "Demo" : "Online");
    
    ps2_sleep(500);
    print_border();
    
    /* Show initial prompt */
    scr_printf("\x1b[36m[ AWAITING INPUT ]\x1b[0m\n");
    scr_printf("(Type 'quit' to exit)\n\n");
    
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
        
        /* Skip empty input - show prompt again, don't process */
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
            scr_printf("|   demo    - Toggle demo mode                 |\n");
            scr_printf("|   quit    - Exit session                     |\n");
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
        
        /* Show processing */
        scr_printf("\x1b[33m[ Processing... ]\x1b[0m\n");
        
        /* Get response */
        if (demo_mode) {
            /* Demo mode - return response then stop at prompt */
            print_border();
            scr_printf("%s\n", demo_responses[demo_index]);
            print_border();
            demo_index = (demo_index + 1) % DEMO_RESPONSES;
        } else {
            /* Online mode */
            if (chat_request(prompt, api_key, model, response, sizeof(response)) == 0) {
                print_border();
                scr_printf("%s\n", response);
                print_border();
            } else {
                print_border();
                scr_printf("| [ERROR] Request failed - network issue?    |\n");
                scr_printf("| Type 'demo' to switch to offline mode       |\n");
                print_border();
            }
        }
        
        /* Show prompt again - this is where it stops */
        scr_printf("\x1b[36m[ AWAITING INPUT ]\x1b[0m\n");
        
        /* Loop continues - but waits for actual input next time */
        /* Empty stdin will just show prompt again (no auto-loop) */
    }
    
    return 0;
}
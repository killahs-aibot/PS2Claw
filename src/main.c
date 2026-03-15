/*
 * PS2Claw - AI Assistant for PlayStation 2
 * gsKit Enhanced Version - Post-Apocalyptic Terminal
 * 
 * Build: make
 * Target: PlayStation 2 (MIPS R5900) via FreeMCBoot
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <curl/curl.h>
#include <debug.h>
#include <gsKit.h>
#include <gsFontM.h>
#include <gsToolkit.h>

/* API Configuration */
#define API_URL "https://openrouter.ai/api/v1/chat/completions"
#define DEFAULT_MODEL "google/gemini-2.0-flash-001"
#define MAX_PROMPT 1024
#define MAX_RESPONSE 4096

/* Color definitions - Green phosphor palette */
#define COLOR_BG         0x00100010    /* Very dark green/black */
#define COLOR_PHOSPHOR   0x0000FF00    /* Bright green */
#define COLOR_DIM        0x00008800    /* Dim green */
#define COLOR_BORDER     0x0000CC00    /* Border green */

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

/* External function declarations for gsKit */
extern void gsKit_hires_flip(GSGLOBAL *gsGlobal);

/* GS Global and Font */
static GSGLOBAL *gsGlobal = NULL;
static GSFONTM *gsFont = NULL;

/* Draw CRT border - simple frame */
void draw_crt_border(GSGLOBAL *gsGlobal) {
    int i;
    u64 border_color = COLOR_BORDER;
    
    /* Top border - solid line */
    for (i = 0; i < 640; i += 2) {
        gsKit_prim_point(gsGlobal, i, 0, 2, border_color);
        gsKit_prim_point(gsGlobal, i+1, 0, 2, border_color);
    }
    
    /* Bottom border */
    for (i = 0; i < 640; i += 2) {
        gsKit_prim_point(gsGlobal, i, 479, 2, border_color);
        gsKit_prim_point(gsGlobal, i+1, 479, 2, border_color);
    }
    
    /* Left border */
    for (i = 0; i < 480; i += 2) {
        gsKit_prim_point(gsGlobal, 0, i, 2, border_color);
        gsKit_prim_point(gsGlobal, 0, i+1, 2, border_color);
    }
    
    /* Right border */
    for (i = 0; i < 480; i += 2) {
        gsKit_prim_point(gsGlobal, 639, i, 2, border_color);
        gsKit_prim_point(gsGlobal, 639, i+1, 2, border_color);
    }
}

/* Draw corner decorations - retro BBS style brackets */
void draw_corner_decorations(GSGLOBAL *gsGlobal) {
    u64 color = COLOR_PHOSPHOR;
    int len = 25;
    
    /* Top-left: ┌ */
    gsKit_prim_point(gsGlobal, 10, 10, 1, color);
    gsKit_prim_point(gsGlobal, 10, 11, 1, color);
    gsKit_prim_point(gsGlobal, 10, 10+len, 1, color);
    gsKit_prim_point(gsGlobal, 10, 10+len-1, 1, color);
    gsKit_prim_point(gsGlobal, 10+len, 10, 1, color);
    gsKit_prim_point(gsGlobal, 10+len-1, 10, 1, color);
    
    /* Top-right: ┐ */
    gsKit_prim_point(gsGlobal, 630, 10, 1, color);
    gsKit_prim_point(gsGlobal, 630, 11, 1, color);
    gsKit_prim_point(gsGlobal, 630, 10+len, 1, color);
    gsKit_prim_point(gsGlobal, 630, 10+len-1, 1, color);
    gsKit_prim_point(gsGlobal, 630-len, 10, 1, color);
    gsKit_prim_point(gsGlobal, 630-len+1, 10, 1, color);
    
    /* Bottom-left: └ */
    gsKit_prim_point(gsGlobal, 10, 470, 1, color);
    gsKit_prim_point(gsGlobal, 10, 469, 1, color);
    gsKit_prim_point(gsGlobal, 10, 470-len, 1, color);
    gsKit_prim_point(gsGlobal, 10, 470-len+1, 1, color);
    gsKit_prim_point(gsGlobal, 10+len, 470, 1, color);
    gsKit_prim_point(gsGlobal, 10+len-1, 470, 1, color);
    
    /* Bottom-right: ┘ */
    gsKit_prim_point(gsGlobal, 630, 470, 1, color);
    gsKit_prim_point(gsGlobal, 630, 469, 1, color);
    gsKit_prim_point(gsGlobal, 630, 470-len, 1, color);
    gsKit_prim_point(gsGlobal, 630, 470-len+1, 1, color);
    gsKit_prim_point(gsGlobal, 630-len, 470, 1, color);
    gsKit_prim_point(gsGlobal, 630-len+1, 470, 1, color);
}

/* Draw scanlines - horizontal dotted lines */
void draw_scanlines(GSGLOBAL *gsGlobal) {
    int y;
    u64 color = COLOR_DIM;
    
    /* Draw horizontal lines every 4 pixels for scanline effect */
    for (y = 2; y < 478; y += 4) {
        int x;
        for (x = 2; x < 638; x += 8) {
            gsKit_prim_point(gsGlobal, x, y, 1, color);
        }
    }
}

/* Print ASCII art header - Enhanced version */
void print_header_gs(GSGLOBAL *gsGlobal, GSFONTM *font) {
    const char *art[] = {
        "  ___ ___ ___   ___ ____     ___ ____ ",
        " |   |   |___| |     |    |___| |    \\",
        " |___|___|   | |_____|___|    | |___/ ",
        "                                        ",
        "   L L L     C C C     W W W          ",
        "   L L L   C       C   W   W          ",
        "   L L L   C         C  W   W          ",
        "   L L L   C         C  W   W          ",
        "   L L L   C       C   W   W          ",
        "   L L L     C C C     W W W          ",
        NULL
    };
    
    int start_y = 60;
    int line = 0;
    
    /* Draw title with glow effect - multiple passes */
    while (art[line]) {
        /* Dim pass */
        gsKit_fontm_print_scaled(gsGlobal, font, 40, start_y + (line * 18), 3, 
            1.0f, COLOR_DIM, (char*)art[line]);
        line++;
    }
    
    /* Bright pass - offset slightly for glow */
    line = 0;
    while (art[line]) {
        gsKit_fontm_print_scaled(gsGlobal, font, 42, start_y + (line * 18) - 1, 3, 
            1.0f, COLOR_PHOSPHOR, (char*)art[line]);
        line++;
    }
    
    /* Version text */
    gsKit_fontm_print_scaled(gsGlobal, font, 210, 260, 3, 0.9f, COLOR_PHOSPHOR, 
        "=== AI Assistant for PS2 ===");
    gsKit_fontm_print_scaled(gsGlobal, font, 240, 280, 3, 0.7f, COLOR_DIM, 
        "v0.2.0 - gsKit Edition");
    gsKit_fontm_print_scaled(gsGlobal, font, 190, 300, 3, 0.6f, COLOR_DIM, 
        "Post-Apocalyptic Terminal Mode");
}

/* Boot sequence animation */
void boot_sequence(GSGLOBAL *gsGlobal, GSFONTM *font) {
    char buffer[128];
    int y = 340;
    int i;
    volatile int delay_count;
    
    /* Phase 1: System boot messages */
    const char *boot_msgs[] = {
        "[BIOS] Initializing GSKit rendering...",
        "[VRAM] Allocating framebuffer 640x480",
        "[GPU]  Graphics synth initialized",
        "[NET]  Network stack loading...",
        "[SYS]  All systems nominal",
        NULL
    };
    
    i = 0;
    while (boot_msgs[i]) {
        snprintf(buffer, sizeof(buffer), "%s", boot_msgs[i]);
        
        /* Draw message with typing effect simulation - draw character by character */
        int len = strlen(buffer);
        char partial[128];
        for (int c = 0; c <= len; c++) {
            partial[c] = buffer[c];
            partial[c+1] = '\0';
        }
        
        /* Show message */
        gsKit_fontm_print_scaled(gsGlobal, font, 50, y, 3, 0.7f, COLOR_PHOSPHOR, buffer);
        
        /* Flip and wait a bit */
        gsKit_queue_exec(gsGlobal);
        gsKit_hires_flip(gsGlobal);
        
        /* Simple delay */
        for (delay_count = 0; delay_count < 300000; delay_count++) { }
        
        y += 16;
        i++;
    }
    
    /* Small pause */
    for (delay_count = 0; delay_count < 500000; delay_count++) { }
}

/* Display boot screen with all effects */
void display_boot_screen(GSGLOBAL *gsGlobal, GSFONTM *font) {
    /* Clear to dark background */
    gsKit_clear(gsGlobal, COLOR_BG);
    
    /* Draw CRT border */
    draw_crt_border(gsGlobal);
    
    /* Draw corner decorations */
    draw_corner_decorations(gsGlobal);
    
    /* Draw scanline effect */
    draw_scanlines(gsGlobal);
    
    /* Draw ASCII header */
    print_header_gs(gsGlobal, font);
    
    /* Run boot sequence */
    boot_sequence(gsGlobal, font);
    
    /* Final flip to show everything */
    gsKit_queue_exec(gsGlobal);
    gsKit_hires_flip(gsGlobal);
}

/* Print border (debug screen version) */
void print_border(void) {
    scr_printf("+");
    for (int i = 0; i < 60; i++) scr_printf("-");
    scr_printf("+\n");
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
    
    /* Build JSON payload - escape special chars in prompt */
    char escaped_prompt[MAX_PROMPT * 2];
    char *ep = escaped_prompt;
    const char *p = prompt;
    while (*p && (ep - escaped_prompt) < (int)(sizeof(escaped_prompt) - 2)) {
        if (*p == '"' || *p == '\\') *ep++ = '\\';
        *ep++ = *p++;
    }
    *ep = 0;
    
    json_payload = malloc(512 + strlen(escaped_prompt));
    if (!json_payload) {
        free(buf.data);
        return -1;
    }
    
    snprintf(json_payload, 512 + strlen(escaped_prompt),
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
    
    scr_printf("[NET] Connecting to %s...\n", API_URL);
    
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
        scr_printf("[ERR] HTTP %ld: %s\n", http_code, buf.data);
        free(buf.data);
        free(json_payload);
        return -1;
    }
    
    scr_printf("[NET] Response received (%zu bytes)\n", buf.size);
    
    /* Extract content */
    content = extract_content(buf.data);
    if (content) {
        strncpy(response, content, resp_size - 1);
        response[resp_size - 1] = 0;
    } else {
        scr_printf("[ERR] Could not parse JSON response\n");
        free(buf.data);
        free(json_payload);
        return -1;
    }
    
    free(buf.data);
    free(json_payload);
    return 0;
}

/* Show GS screen message */
void show_gs_message(GSGLOBAL *gsGlobal, GSFONTM *font, const char *text) {
    gsKit_clear(gsGlobal, COLOR_BG);
    draw_crt_border(gsGlobal);
    draw_corner_decorations(gsGlobal);
    draw_scanlines(gsGlobal);
    
    gsKit_fontm_print_scaled(gsGlobal, font, 100, 220, 3, 1.0f, COLOR_PHOSPHOR, (char*)text);
    
    gsKit_queue_exec(gsGlobal);
    gsKit_hires_flip(gsGlobal);
}

/* Main program */
int main(int argc, char *argv[]) {
    char *api_key;
    char *model;
    char prompt[MAX_PROMPT];
    char response[MAX_RESPONSE];
    
    /* Initialize debug screen first (fallback output) */
    init_scr();
    scr_printf("PS2Claw v0.2.0 - gsKit Edition\n");
    scr_printf("Initializing...\n");
    
    /* Initialize gsKit */
    gsGlobal = gsKit_init_global();
    if (!gsGlobal) {
        scr_printf("[ERR] Failed to initialize gsKit\n");
        return 1;
    }
    
    /* Initialize screen mode - NTSC 640x480 */
    gsKit_init_screen(gsGlobal);
    
    /* Initialize font */
    gsFont = gsKit_init_fontm();
    if (!gsFont) {
        scr_printf("[WARN] Font init failed, using fallback\n");
    }
    
    /* Upload default font (built-in) */
    if (gsFont) {
        gsKit_fontm_upload(gsGlobal, gsFont);
    }
    
    scr_printf("Display initialized!\n");
    
    /* Show boot screen with effects */
    display_boot_screen(gsGlobal, gsFont);
    
    /* Now use debug screen for interactive part */
    print_border();
    scr_printf("  POST-APOCALYPTIC TERMINAL MODE\n");
    scr_printf("  > Green phosphor display active\n");
    scr_printf("  > Scanlines enabled\n");
    scr_printf("  > CRT border rendered\n");
    print_border();
    scr_printf("\n");
    
    /* Get API key from environment */
    api_key = getenv("OPENROUTER_API_KEY");
    if (!api_key) {
        show_gs_message(gsGlobal, gsFont, "[ERR] OPENROUTER_API_KEY not set");
        
        scr_printf("[ERR] OPENROUTER_API_KEY not set\n\n");
        scr_printf("Usage:\n");
        scr_printf("  export OPENROUTER_API_KEY=your_key\n");
        scr_printf("  ./PS2CLAW.ELF\n\n");
        scr_printf("Optional:\n");
        scr_printf("  export MODEL=google/gemini-2.0-flash-001\n");
        return 1;
    }
    
    /* Get model */
    model = getenv("MODEL");
    if (!model) model = DEFAULT_MODEL;
    
    scr_printf("[CFG] Model: %s\n", model);
    scr_printf("[CFG] API: %s\n\n", API_URL);
    
    scr_printf("Ready! Type your message below.\n");
    scr_printf("(Type 'quit' or 'exit' to end session)\n\n");
    
    /* Interactive loop */
    while (1) {
        scr_printf("> ");
        
        /* Read input - use fgets via stdin */
        if (!fgets(prompt, sizeof(prompt), stdin)) break;
        
        /* Remove newline */
        prompt[strcspn(prompt, "\n")] = 0;
        
        if (strcmp(prompt, "quit") == 0 || strcmp(prompt, "exit") == 0) {
            /* Show exit screen on GS */
            show_gs_message(gsGlobal, gsFont, "Session terminated. -=[ PS2CLAW ]=-");
            
            scr_printf("\n");
            print_border();
            scr_printf("  Session terminated.\n");
            scr_printf("  -=[ PS2CLAW ]=-\n");
            print_border();
            break;
        }
        
        if (strlen(prompt) > 0) {
            scr_printf("\n");
            
            if (chat_request(prompt, api_key, model, response, sizeof(response)) == 0) {
                print_border();
                scr_printf("%s\n", response);
                print_border();
            } else {
                print_border();
                scr_printf("  [ERROR] Request failed\n");
                scr_printf("  Check network connection\n");
                print_border();
            }
            
            scr_printf("\n");
        }
    }
    
    /* Cleanup */
    if (gsFont) {
        gsKit_free_fontm(gsGlobal, gsFont);
    }
    if (gsGlobal) {
        gsKit_deinit_global(gsGlobal);
    }
    
    return 0;
}
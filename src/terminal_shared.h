/*
 * PS2Claw - Shared Variables for Terminal Mode
 * External declarations for providers, models, etc.
 */

#ifndef __PS2CLAW_TERMINAL_SHARED_H__
#define __PS2CLAW_TERMINAL_SHARED_H__

/* Provider count */
#define MAX_PROVIDERS 5

/* Provider types */
typedef enum {
    AUTH_BEARER,
    AUTH_API_KEY
} AuthType;

/* Provider struct - must match main.c */
typedef struct {
    const char *name;
    const char *id;
    char api_key[128];
    char model[64];
    const char *endpoint;
    AuthType auth_type;
    int enabled;
} Provider;

/* External variables from main.c */
extern Provider providers[MAX_PROVIDERS];
extern int current_provider;
extern int num_providers;
extern int demo_mode;

/* Print functions from main.c - for menu.c compatibility */
void print_border(void);
void print_provider_footer(void);

/* Get current provider */
Provider* get_current_provider(void);

/* Cycle provider */
void cycle_provider(void);

#endif /* __PS2CLAW_TERMINAL_SHARED_H__ */
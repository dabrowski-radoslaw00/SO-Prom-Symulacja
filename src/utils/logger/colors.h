#ifndef COLORS_H
#define COLORS_H

/* Kolory tekstu */
#define COLOR_RESET   "\033[0m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_WHITE   "\033[37m"

#define COLOR_BRIGHT_RED     "\033[91m"
#define COLOR_BRIGHT_GREEN   "\033[92m"
#define COLOR_BRIGHT_YELLOW  "\033[93m"
#define COLOR_BRIGHT_BLUE    "\033[94m"
#define COLOR_BRIGHT_MAGENTA "\033[95m"
#define COLOR_BRIGHT_CYAN    "\033[96m"

/* Style tekstu */
#define STYLE_BOLD "\033[1m"
#define STYLE_DIM  "\033[2m"

/* Style tekstu */
#define C_SUCCESS COLOR_BRIGHT_GREEN    // Operacje zakończone sukcesem
#define C_ERROR   COLOR_BRIGHT_RED      // Błędy krytyczne
#define C_WARNING COLOR_BRIGHT_YELLOW   // Ostrzeżenia
#define C_INFO    COLOR_BRIGHT_CYAN     // Informacje ogólne
#define C_VIP     COLOR_BRIGHT_MAGENTA  // Pasażerowie VIP
#define C_MALE    COLOR_BRIGHT_BLUE     // Pasażerowie męscy
#define C_FEMALE  COLOR_BRIGHT_MAGENTA  // Pasażerki

#endif
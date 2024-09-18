// Language definitions.
// First is english: then it is in order of the ACPMetaXml struct
// https://wut.devkitpro.org/group__nn__acp__title.html#structACPMetaXml
#define LANG_ENGLISH             0
#define LANG_JAPANESE            1
#define LANG_FRENCH              2
#define LANG_GERMAN              3
#define LANG_ITALIAN             4
#define LANG_SPANISH             5
#define LANG_SIMPLIFIED_CHINESE  6
#define LANG_KOREAN              7
#define LANG_DUTCH               8
#define LANG_PORTUGUESE          9
#define LANG_RUSSIAN             10
#define LANG_TRADITIONAL_CHINESE 11

#define TITLE_LANG_DEFAULT_VALUE LANG_ENGLISH
static uint32_t titleLang = TITLE_LANG_DEFAULT_VALUE;

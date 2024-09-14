#include <nn/acp/title.h>
#include <sysapp/launch.h>
#include <sysapp/title.h>
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
uint32_t titleLang = TITLE_LANG_DEFAULT_VALUE;

inline char *getTitleLongname(ACPMetaXml *meta) {
    char *ret;
    switch (titleLang) {
        case LANG_JAPANESE:
            ret = meta->longname_ja;
            break;
        case LANG_FRENCH:
            ret = meta->longname_fr;
            break;
        case LANG_GERMAN:
            ret = meta->longname_de;
            break;
        case LANG_ITALIAN:
            ret = meta->longname_it;
            break;
        case LANG_SPANISH:
            ret = meta->longname_es;
            break;
        case LANG_SIMPLIFIED_CHINESE:
            ret = meta->longname_zhs;
            break;
        case LANG_KOREAN:
            ret = meta->longname_ko;
            break;
        case LANG_DUTCH:
            ret = meta->longname_nl;
            break;
        case LANG_PORTUGUESE:
            ret = meta->longname_pt;
            break;
        case LANG_RUSSIAN:
            ret = meta->longname_ru;
            break;
        case LANG_TRADITIONAL_CHINESE:
            ret = meta->longname_zht;
            break;
        case LANG_ENGLISH:
        default:
            ret = meta->longname_en;
            break;
    }

    // Fallback for titles which don't have a language-specific translation
    if (ret != NULL && ret[0] == '\0') ret = meta->longname_en;
    return ret;
}

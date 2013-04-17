/**
 * Modified from nederlands.c by hudson (i don't really know C)
 * https://bitbucket.org/hudson/pebble/src/fed432595247/hoelaat/src/nederlands.c?at=default
 */
#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define UUID {0xB2, 0x30, 0x63, 0xA6, 0x7E, 0xE2, 0x4C, 0x84, 0x97, 0xA1, 0x8F, 0x87, 0x87, 0xD0, 0x7E, 0x18},
PBL_APP_INFO(
    UUID,
    "Fuzzy Norsk 3",
    "olemartinorg",
    1, 0, /* App version */
    RESOURCE_ID_IMAGE_MENU_ICON,
    APP_INFO_WATCH_FACE
);

static Window window;
static GFont font_tiny;
static GFont font_thin;
static GFont font_thick;

typedef struct {
    TextLayer layer;
    PropertyAnimation anim;
    const char * text;
    const char * old_text;
} word_t;

static word_t line1;
static word_t line2;
static word_t line3;
static word_t date_line;
static char date_str_buf[80];

static const char * hours[] = {
    "",
    "ett",
    "to",
    "tri",
    "fira",
    "fem",
    "seks",
    "sju",
    "åtta",
    "ni",
    "ti",
    "elleve",
    "tolv"
};

static const char * days[] = {
    "Søn",
    "Man",
    "Tirs",
    "Ons",
    "Tors",
    "Fre",
    "Lør"
};

static const char * months[] = {
    "",
    "Jan",
    "Feb",
    "Mars",
    "April",
    "Mai",
    "Juni",
    "Juli",
    "Aug",
    "Sept",
    "Okt",
    "Nov",
    "Des"
};

// Thanks to http://stackoverflow.com/a/10011878
void get_dec_str(char * str, int len, int val){
    int i;

    for(i=1; i<=len; i++){
        str[len-i] = (char) ((val % 10UL) + '0');
        val/=10;
    }

    str[i-1] = '\0';
}

static void norsk_fuzzy(int h, int m, int s){
    /*
    h = 10;
    m = 36;
    */
    // minutes
    if      ((m == 57 && s >= 30) || m > 57) { line1.text = "";         line2.text = "";      h++; }
    else if ((m == 52 && s >= 30) || m > 52) { line1.text = "fem";      line2.text = "på";    h++; }
    else if ((m == 47 && s >= 30) || m > 47) { line1.text = "ti";       line2.text = "på";    h++; }
    else if ((m == 42 && s >= 30) || m > 42) { line1.text = "kvart";    line2.text = "på";    h++; }
    else if ((m == 37 && s >= 30) || m > 37) { line1.text = "ti over";  line2.text = "halv";  h++; }
    else if ((m == 32 && s >= 30) || m > 32) { line1.text = "fem over"; line2.text = "halv";  h++; }
    else if ((m == 27 && s >= 30) || m > 27) { line1.text = "";         line2.text = "halv";  h++; }
    else if ((m == 22 && s >= 30) || m > 22) { line1.text = "fem på";   line2.text = "halv";  h++; }
    else if ((m == 17 && s >= 30) || m > 17) { line1.text = "ti på";    line2.text = "halv";  h++; }
    else if ((m == 12 && s >= 30) || m > 12) { line1.text = "kvart";    line2.text = "over";       }
    else if ((m ==  7 && s >= 30) || m >  7) { line1.text = "ti";       line2.text = "over";       }
    else if ((m ==  2 && s >= 30) || m >  2) { line1.text = "fem";      line2.text = "over";       }
    else {                                     line1.text = "";         line2.text = "";           }

    // hour
    if (h < 12)  line3.text = hours[h];
    else         line3.text = hours[h - 12];
    if (h == 0 || h == 12)
        line3.text = hours[12];
}

static void norsk_dato(int month, int date, int day){
    date_str_buf[0] = '\0';

    char date_buf[3];
    if (date > 9)   get_dec_str(date_buf, 2, date);
    else            get_dec_str(date_buf, 1, date);

    strcat(date_str_buf, days[day]);
    strcat(date_str_buf, " ");
    strcat(date_str_buf, date_buf);
    strcat(date_str_buf, ". ");
    strcat(date_str_buf, months[month]);

    date_line.text = date_str_buf;
}

static void display_time(PblTm * const ptm){
    line1.old_text = line1.text;
    line2.old_text = line2.text;
    line3.old_text = line3.text;

    norsk_fuzzy(ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    norsk_dato(ptm->tm_mon+1, ptm->tm_mday, ptm->tm_wday);

    text_layer_set_text(&line1.layer, line1.text);
    text_layer_set_text(&line2.layer, line2.text);
    text_layer_set_text(&line3.layer, line3.text);
    text_layer_set_text(&date_line.layer, date_line.text);
}

static void handle_tick(AppContextRef ctx, PebbleTickEvent * const event){
    (void) ctx;
    if (event->tick_time->tm_sec != 30 &&
        event->tick_time->tm_sec != 0)
        return;

    PblTm * ptm = event->tick_time;
    display_time(ptm);
}

void text_layer(word_t * word, GRect frame, GFont font){
    text_layer_init(&word->layer, frame);
    text_layer_set_text(&word->layer, "");
    text_layer_set_text_color(&word->layer, GColorWhite);
    text_layer_set_background_color(&word->layer, GColorClear);
    text_layer_set_font(&word->layer, font);
    layer_add_child(&window.layer, &word->layer.layer);
}

static void handle_init(AppContextRef ctx){
    (void) ctx;

    window_init(&window, "Main");
    window_stack_push(&window, true);
    window_set_background_color(&window, GColorBlack);

    resource_init_current_app(&APP_RESOURCES);

    font_tiny  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TINY_22));
    font_thin  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_THIN_33));
    font_thick = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_THICK_33));

    text_layer(&line1,     GRect(4, 8,   140, 40), font_thin);
    text_layer(&line2,     GRect(4, 43,  140, 40), font_thin);
    text_layer(&line3,     GRect(4, 78,  140, 40), font_thick);
    text_layer(&date_line, GRect(4, 127, 140, 40), font_tiny);

    PblTm tick_time;
    get_time(&tick_time);
    display_time(&tick_time);
}

static void handle_deinit(AppContextRef ctx){
    (void) ctx;

    fonts_unload_custom_font(font_tiny);
    fonts_unload_custom_font(font_thin);
    fonts_unload_custom_font(font_thick);
}

void pbl_main(void * const params) {
    PebbleAppHandlers handlers = {
        .init_handler = &handle_init,
        .deinit_handler = &handle_deinit,
        .tick_info  = {
            .tick_handler = &handle_tick,
            .tick_units = SECOND_UNIT,
        },
    };

    app_event_loop(params, &handlers);
}

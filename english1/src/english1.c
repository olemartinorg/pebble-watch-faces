#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define UUID {0xA8, 0xD3, 0x6C, 0xED, 0x7A, 0xB0, 0x42, 0x3F, 0xAB, 0x3E, 0x0E, 0xCD, 0x8E, 0xDF, 0xBC, 0xA3},
PBL_APP_INFO(
    UUID,
    "Fuzzy English",
    "erget",
    1, 0, // Version
    RESOURCE_ID_IMAGE_MENU_ICON,
    APP_INFO_WATCH_FACE
);

static Window window;
static GFont font_thin;
static GFont font_thick;

typedef struct {
    TextLayer layer;
    PropertyAnimation anim;
    const char * text;
    const char * old_text;
} word_t;

static word_t fuzzy_word;
static word_t time_word;
static word_t date_word;
static word_t day_word;
static char date_str_buf[80];
static char fuzzy_str_buf[80];

static const char * hours[] = {
    "",
    "one",
    "two",
    "three",
    "four",
    "five",
    "six",
    "seven",
    "eight",
    "nine",
    "ten",
    "eleven",
    "twelve"
};

static const char * days[] = {
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat"
};

static const char * months[] = {
    "",
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
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
    fuzzy_str_buf[0] = '\0';
    bool space = true;

    if      ((m == 57 && s >= 30) || m > 57) { space = false;                          h++; }
    else if ((m == 52 && s >= 30) || m > 52) { strcat(fuzzy_str_buf, "five till");     h++; }
    else if ((m == 47 && s >= 30) || m > 47) { strcat(fuzzy_str_buf, "ten till");      h++; }
    else if ((m == 42 && s >= 30) || m > 42) { strcat(fuzzy_str_buf, "quarter till");  h++; }
    else if ((m == 37 && s >= 30) || m > 37) { strcat(fuzzy_str_buf, "twenty till");   h++; }
//    else if ((m == 32 && s >= 30) || m > 32) { strcat(fuzzy_str_buf, "fem over halv"); h++; }
    else if ((m == 27 && s >= 30) || m > 27) { strcat(fuzzy_str_buf, "half past");     h; }
//    else if ((m == 22 && s >= 30) || m > 22) { strcat(fuzzy_str_buf, "fem på halv");   h++; }
    else if ((m == 17 && s >= 30) || m > 17) { strcat(fuzzy_str_buf, "twenty past");   h; }
    else if ((m == 12 && s >= 30) || m > 12) { strcat(fuzzy_str_buf, "quarter past");       }
    else if ((m ==  7 && s >= 30) || m >  7) { strcat(fuzzy_str_buf, "ten past");           }
    else if ((m ==  2 && s >= 30) || m >  2) { strcat(fuzzy_str_buf, "five past");          }
    else {                                     space = false;                               }

    if (space)
        strcat(fuzzy_str_buf, " ");

    const char * hour;

    if (h < 12)  hour = hours[h];
    else         hour = hours[h - 12];
    if (h == 0 || h == 12)
        hour = hours[12];

    strcat(fuzzy_str_buf, hour);
    fuzzy_word.text = fuzzy_str_buf;
}

static void norsk_dato(int month, int date, int day){
    date_str_buf[0] = '\0';

    char date_buf[3];
    if (date > 9)   get_dec_str(date_buf, 2, date);
    else            get_dec_str(date_buf, 1, date);

    strcat(date_str_buf, date_buf);
    strcat(date_str_buf, " ");
    strcat(date_str_buf, months[month]);

    day_word.text = days[day];
    date_word.text = date_str_buf;
}


static void display_time(PblTm * const ptm){
    date_word.old_text = date_word.text;
    day_word.old_text = day_word.text;
    time_word.old_text = time_word.text;
    fuzzy_word.old_text = fuzzy_word.text;

    norsk_fuzzy(ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    norsk_dato(ptm->tm_mon+1, ptm->tm_mday, ptm->tm_wday);

    static char time_text[] = "00:00";
    string_format_time(time_text, sizeof(time_text), "%R", ptm);
    time_word.text = time_text;

    text_layer_set_text(&date_word.layer, date_word.text);
    text_layer_set_text(&day_word.layer, day_word.text);
    text_layer_set_text(&time_word.layer, time_word.text);
    text_layer_set_text(&fuzzy_word.layer, fuzzy_word.text);
}

static void handle_tick(AppContextRef ctx, PebbleTickEvent * const event){
    (void) ctx;
    if (event->tick_time->tm_sec != 30 &&
        event->tick_time->tm_sec != 0)
        return;

    PblTm * ptm = event->tick_time;
    display_time(ptm);
}

void text_layer(word_t * word, GRect frame, GFont font, GTextAlignment alignment){
    text_layer_init(&word->layer, frame);
    text_layer_set_text_alignment(&word->layer, alignment);
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

    font_thin = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_23));
    font_thick = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BIG_44));

    text_layer(&day_word,   GRect(4,  0,   140, 30), font_thin,  GTextAlignmentLeft);
    text_layer(&date_word,  GRect(4,  0,   140, 30), font_thin,  GTextAlignmentRight);
    text_layer(&time_word,  GRect(0,  30,  144, 70), font_thick, GTextAlignmentCenter);
    text_layer(&fuzzy_word, GRect(16, 95,  112, 60), font_thin,  GTextAlignmentCenter);

    PblTm tick_time;
    get_time(&tick_time);
    display_time(&tick_time);
}

static void handle_deinit(AppContextRef ctx){
    (void) ctx;

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

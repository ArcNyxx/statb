/* define an error macro or function which takes one char *input */
#define ERR(info) write(STDERR_FILENO, info, sizeof(info) - 1)

/* define maximum bar length */
#define BUF_LEN 256

static const char audio_card[] = "default";
static const char audio_mixer[] = "Master";
static const char audio_mute[] = "Mute";

static const char mem_path[] = "/proc/meminfo";

#define BAT_PATH "/sys/class/power_supply/"
#define BAT "BAT1"
#define BAT_PATH_CAP "/capacity"
#define BAT_PATH_STAT "/status"

static const char internet_unavail[] = "Unavailable";

static const char wifi_card[] = "wlp0s20f3";

#define ORDIND_DATE 8
static char date_fmt[] = "%a %b %dth %H:%M:%S %Z";

static const char sep_open[] = " [", sep_close[] = "]";
static const char name[] = " doggo-dwm ";

#define FUNC(func, iden) { func, iden, sizeof(iden) - 1 }
static const Func funcs[] = {
	FUNC(audio, "Audio: "),
	FUNC(memory, "Memory: "),
	FUNC(battery, "Battery: "),
	FUNC(internet, "Internet: "),
	FUNC(datetime, "Datetime: ")
};

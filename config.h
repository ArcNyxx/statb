/* define an error macro or function which takes one char *input */
#define ERR(info) write(STDERR_FILENO, info, sizeof(info) - 1)

static const char audio_card[] = "default";
static const char audio_mixer[] = "Master";

static const char mem_path[] = "/proc/meminfo";

#define BAT_PATH "/sys/class/power_supply/"
#define BAT "BAT1"
#define BAT_PATH_CAP "/capacity"
#define BAT_PATH_STAT "/status"

static inline char
bat_stat_char(char c)
{
	switch (c) {
	case 'F':
		return 'f';
	case 'C':
		return '+';
	case 'D':
		return '-';
	case 'U':
		return 'x';
	default:
		return 0;
	}
}

static const char internet_unavail[] = "Unavailable";

static const char wifi_card[] = "wlp0s20f3";

#define ORDIND_DATE 8
static char date_fmt[] = "%a %b %dth %H:%M:%S %Z";

static const char sep_open[] = " [", sep_close[] = "]";
static const char name[] = " doggo-dwm ";

static const Func funcs[] = {
	{ audio, "Audio: ", 7 },
	{ memory, "Memory: ", 8 },
	{ battery, "Battery: ", 9 },
	{ internet, "Internet: ", 10 },
	{ datetime, "Datetime: ", 10 }
};

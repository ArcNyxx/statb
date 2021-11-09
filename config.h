/*
 * statb - simple stat bar
 * Copyright (C) 2021 FearlessDoggo21
 * see LICENCE file for licensing information
 */

#define BUF_LEN 256 /* maximum output buffer length */

static const char audio_card[] = "default";
static const char audio_mixer[] = "Master";
static const char audio_mute[] = "Mute";

static const char mem_path[] = "/proc/meminfo";

static const char batcap_path[] = "/sys/class/power_supply/BAT1/capacity";
static const char batstat_path[] = "/sys/class/power_supply/BAT1/status";

/* define a macro to determine whether ip or only status shown */
#define INTERNET_NOSHOW_IP
static const char internet_connect[] = "Connected";
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

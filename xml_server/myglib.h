#ifndef _MYGLIB_H_
#define _MYGLIB_H_

#include <glib.h>

/** @file
 * a set of macros to make programming C easier 
 */

#ifdef UNUSED_PARAM
#elif defined(__GNUC__)
# define UNUSED_PARAM(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED_PARAM(x) /*@unused@*/ x
#else
# define UNUSED_PARAM(x) x
#endif

#define F_SIZE_T "%"G_GSIZE_FORMAT
#define F_U64 "%"G_GUINT64_FORMAT

#define MYGLIB_RESOLUTION_SEC	0x0
#define MYGLIB_RESOLUTION_MS	0x1

#define MYGLIB_RESOLUTION_DEFAULT	MYGLIB_RESOLUTION_SEC

typedef struct myglib_log_struct myglib_log_t;
struct myglib_log_struct {
    GLogLevelFlags  min_lvl;

    gchar *         log_filename;
    gint            log_file_fd;

    gboolean        use_syslog;

#ifdef _WIN32
    HANDLE event_source_handle;
    gboolean use_windows_applog;
#endif
    gboolean        rotate_logs;

    GString *       log_ts_str;
    gint	        log_ts_resolution;	/*<< timestamp resolution (sec, ms) */

    GString *       last_msg;
    time_t          last_msg_ts;
    guint           last_msg_count;
};

void
myglib_log_func(
    const gchar*        log_domain, 
    GLogLevelFlags      log_level, 
    const gchar *       message, 
    gpointer            user_data                    
);

myglib_log_t *myglib_log_init(void) G_GNUC_DEPRECATED;
myglib_log_t *myglib_log_new(void);
int myglib_log_set_level(myglib_log_t *log, const gchar *level);
char* myglib_log_get_level_name(int	log_level);		//add by vinchen/CFR
void myglib_log_free(myglib_log_t *log);
int myglib_log_open(myglib_log_t *log);
void myglib_log_func(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data);
void myglib_log_set_logrotate(myglib_log_t *log);
int myglib_log_set_event_log(myglib_log_t *log, const char *app_name);
const char *myglib_log_skip_topsrcdir(const char *message);
void myglib_log_set_logtimestamp_resolution(myglib_log_t *log, int res);
int myglib_log_get_logtimestamp_resolution(myglib_log_t *log);


#endif
/* source: http://www.jera.com/techinfo/jtns/jtn002.html */

#define xstr(x) str(x)
#define str(x) #x

/* file: minunit.h */
#define mu_assert(message, test) do { if (!(test)) return __FILE__ "(" xstr(__LINE__) "): " message; } while (0)
#define mu_run_test(test) do { char *message = test(); tests_run++; if (message) return message; } while (0)
extern int tests_run;


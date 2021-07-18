#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

unsigned get_seconds(char *clock) {
    unsigned ret = 0;
    ret += 10*3600 * (clock[0] - '0'); ret +=  1*3600 * (clock[1] - '0');
    ret += 10*  60 * (clock[3] - '0'); ret +=  1*  60 * (clock[4] - '0');
    ret += 10*   1 * (clock[6] - '0'); ret +=  1*   1 * (clock[7] - '0');
    return ret;
}

struct clock {
    unsigned t;
    unsigned d;
};

//#define DO_SORT
#ifdef DO_SORT
int cmp_clock(const void *a, const void *b) {
    const struct clock *A, *B;

    A = (const struct clock*)a;
    B = (const struct clock*)b;
    if (A->d != B->d)
        return A->d - B->d;
    return A->t - B->t;
}
#endif

int main() {
    int tc;
    scanf("%d",&tc);
    while (tc--) {
        int n;
        unsigned display[86400];
        unsigned after[2][86400];
        unsigned cur_slot;
        unsigned after_count;

        scanf("%d",&n);
        struct clock c[n];

        for (int i=0; i<n; i++) {
            char tmp[10];
            scanf("%s", tmp);
            c[i].t = get_seconds (tmp);
        }

        int skip_all = 0;
        for (int i=0; i<n; i++) {
            int _d;

            scanf("%d",&_d);
            if (skip_all)
                continue;

            _d %= 86400;
            if (_d < 0)
                _d += 86400;
            c[i].d = _d;

#ifndef DO_SORT
            int skip_same = 0;
            for (int j=0; j<i; j++) {
                if (c[j].d == c[i].d) {
                    if (c[j].t != c[i].t)
                        skip_all = 1;
                    else
                        skip_same = 1;
                    break;
                }
            }
            if (skip_all) {
                after_count = 0;
                continue;
            }
            if (skip_same)
                continue;

            if (i == 0) {
                cur_slot = 0;
                after_count = 86400;

                unsigned tmp_t = c[0].t;
                for (int j=0; j<after_count; j++) {
                    after[cur_slot][j] = j;

                    display[j] = tmp_t;
                    tmp_t += c[0].d;
                    tmp_t %= 86400;
                }
            }
            else {
                unsigned next_after_count = 0;
                unsigned next_slot = (cur_slot + 1 ) % 2;

                for (int j=0; j<after_count; j++) {
                    unsigned aft = after[cur_slot][j];
                    unsigned cur_display;
                    cur_display = (1ULL * c[i].t + 1ULL * aft * c[i].d) % 86400;
                    if (display[aft] == cur_display) {
                        after[next_slot][next_after_count] = aft;
                        next_after_count ++;
                    }
                }

                after_count = next_after_count;
                cur_slot = next_slot;

                if (after_count == 0)
                    break;
            }
#endif
        }

#ifdef DO_SORT
        qsort (c, n, sizeof (c[0]), cmp_clock);
        int no_match = 0;
        for (int j=1; j<n; j++) {
            if (c[j].d == c[j-1].d) {
                if (c[j].t != c[j-1].t) {
                    no_match = 1;
                    break;
                }

                memmove (c+j, c+j+1, sizeof(c[0])*(n-j-1));

                j --;
                n --;
            }
        }
        if (no_match) {
            printf ("%d\n", 0);
            continue;
        }

        cur_slot = 0;
        after_count = 86400;

        unsigned tmp_t = c[0].t;
        for (int j=0; j<after_count; j++) {
            after[cur_slot][j] = j;

            display[j] = tmp_t;
            tmp_t += c[0].d;
            tmp_t %= 86400;
        }

        for (int i=1; i<n; i++) {
            unsigned next_after_count = 0;
            unsigned next_slot = (cur_slot + 1 ) % 2;

            for (int j=0; j<after_count; j++) {
                unsigned aft = after[cur_slot][j];
                unsigned cur_display;
                cur_display = (1ULL * c[i].t + 1ULL * aft * c[i].d) % 86400;
                if (display[aft] == cur_display) {
                    after[next_slot][next_after_count] = aft;
                    next_after_count ++;
                }
            }

            after_count = next_after_count;
            cur_slot = next_slot;

            if (after_count == 0)
                break;
        }
#endif

        printf ("%d\n", after_count);
    }
    return 0;
}

// vim:set sw=4 et:

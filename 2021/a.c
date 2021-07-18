#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct simgroup
{
    unsigned sim_count;
    char str[1+20+1];
};

unsigned append(struct simgroup *list, unsigned count, char *str)
{
    unsigned s, e, m;

    s = 0;
    e = count;
    m = (s + e) / 2;

    while (s < e)
    {
        int cmp;

        //printf("%d%d%d\n", s, m, e);
        cmp = strcmp (list[m].str, str);
        if (cmp == 0)
        {
            list[m].sim_count ++;
            return count;
        }

        if (cmp < 0)
            s = m+1;
        else
            e = m;

        m = (s + e) / 2;
    }

    if (count != m)
        memmove (list+(m+1), list+m, sizeof(list[0]) * (count-m));
    list[m].sim_count = 1;
    strcpy (list[m].str, str);

    //printf ("new at %2d, \"%s\"\n", m, str);
    //for (int i=0; i<count+1; i++)
    //    printf ("%4d, sim%2d \"%s\"\n", i, list[i].sim_count, list[i].str);

    return ++count;
}

int cmp_char (const void *a, const void *b)
{
    char A = *(const char*)a;
    char B = *(const char*)b;

    return A - B;
}

int main()
{
    int t;

    scanf("%d",&t);
    while (t--)
    {
        int n, k;

        scanf("%d %d",&n, &k);

        struct simgroup list[n];
        unsigned scount = 0;
        unsigned sim;

        for (int i=0; i<n; i++) {
            int j;
            char str[1+k+1];

            scanf("%s", &str[1]);
            str[0] = 'A';
            for (j=0; j<k; j++)
            {
                if (str[1+j] >= 'a')
                {
                    str[0] ++;
                    str[1+j] -= 'a' - 'A';
                }
            }

            qsort (str+1, k, 1, cmp_char);

            scount = append (list, scount, str);
        }

        sim = 0;
        for (unsigned i=0; i<scount; i++)
        {
            //printf ("%4d, sim%2d \"%s\"\n", i, list[i].sim_count, list[i].str);
            sim += list[i].sim_count * (list[i].sim_count - 1) / 2;
        }

        printf ("%d\n", sim);
    }
    return 0;
}

// vim:set sw=4 et:

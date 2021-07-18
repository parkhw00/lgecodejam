#include <stdio.h>
#include <string.h>

unsigned long long get_value(char *str, unsigned len)
{
    unsigned long long ret = 0;

    while (len -- > 0)
    {
        ret <<= 4;

        if (*str <= '9')
            ret += *str - '0';
        else
            ret += *str - 'A' + 10;

        str ++;
    }

    return ret;
}

int check(char *str, unsigned len,
        unsigned long long value, unsigned offs)
{
    unsigned count = 0;
    unsigned cur_len;
    unsigned long long cur_value;

    if (offs == len)
        return 1;

    for (cur_len = 1; cur_len <= len - offs; cur_len ++)
    {
        cur_value = get_value (str + offs, cur_len);
        if (cur_value >= value)
            count += check(str, len, cur_value, offs + cur_len);
    }

    return count;
}

int main() {
    int t;
    scanf("%d",&t);
    while (t--) {
        char str[20];
        unsigned len;
        unsigned count;

        scanf("%s",str);
        len = strlen (str);

        count = check (str, len, 0, 0);

        printf ("%d\n", count);
    }
    return 0;
}

// vim:set sw=4 et:

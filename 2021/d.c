#include <stdio.h>
int main() {
    int tc;
    scanf("%d",&tc);
    while (tc--) {
        int n, x;
        scanf("%d %d",&n,&x);
        long long hs, ha, hb, hc;
        long long ws, wa, wb, wc;
        scanf("%lld %lld %lld %lld",&hs, &ha, &hb, &hc);
        scanf("%lld %lld %lld %lld",&ws, &wa, &wb, &wc);

        long long H[n], W[n];
        H[0] = hs % hc + 1;
        W[0] = ws % wc + 1;
        for (int i=1; i<=n-1; i++) {
            H[i] = H[i-1] + 1 + (H[i-1] * ha + hb) % hc;
            W[i] = (W[i-1] * wa + wb) % wc + 1;
        }

        // Insert code here


        // End of insertion
    }
    return 0;
}

// vim:set sw=4 et:

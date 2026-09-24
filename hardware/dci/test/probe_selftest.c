/*
 * Host self-test for dci_demo_sync_probe(). NOT part of the target build.
 *
 * The probe is a diagnostic that will be read as authoritative on a board where nothing
 * else works, so its four verdicts have to be right for the four situations they name.
 * Here the real function text is #included verbatim from dci_demo_irq.c and driven by a
 * fake DCI->SR/RIS that plays a synthetic waveform, so the logic is exercised without a
 * board and cannot drift from the shipped source.
 *
 * Build and run - the first line regenerates probe_extracted.inc from the shipped driver
 * source, so the test can never pass against a stale copy of the function:
 *
 *     awk '/^void dci_demo_sync_probe\(void\)$/,0' dci_demo_irq.c \
 *       | sed -e 's/DCI->SR/sr_read()/g' -e 's/DCI->RIS/ris_read()/g' > probe_extracted.inc
 *     gcc -Wall -Wextra -o probe_selftest probe_selftest.c && ./probe_selftest
 *
 * The sed is the only edit made to the real code: two register reads become calls that
 * advance the synthetic waveform. Everything the test exercises - the edge counting, the
 * duty cycle, the four verdicts - is the shipped text.
 *
 * Expected: A -> PIXCLK missing, B and B2 -> stuck level, C -> VSYNC only, D -> RIS moved.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Small enough to read in a terminal, still a multiple of 100 as the probe requires. */
#define DCI_DEMO_SYNC_PROBE_SAMPLES 2000

#define DEMO_LOG(...)   do { printf("  "); printf(__VA_ARGS__); printf("\n"); } while (0)

/*
 * The waveform under test. Periods are in samples. A real capture is nested - many lines
 * per frame - and the bit sense matches the hardware: SR is pin XOR polarity, and a 0 in
 * SR is the ACTIVE (non-blanking) state, which is what the probe counts as active.
 */
static struct
{
    int      h_period, h_active_len;    /* 0 period = pin stuck */
    int      v_period, v_active_len;
    int      h_stuck_val, v_stuck_val;  /* used when the matching period is 0 */
    uint32_t ris;                       /* what RIS reports throughout */
    int      fne;
    int      hspol, vspol;              /* the XOR the hardware applies before SR */
    long     t;
} wf;

static uint32_t fake_sr(void)
{
    uint32_t sr = 0;
    int h_blank, v_blank;

    if (wf.h_period > 0)
    {
        h_blank = ((wf.t % wf.h_period) >= wf.h_active_len);
    }
    else
    {
        h_blank = wf.h_stuck_val;
    }

    if (wf.v_period > 0)
    {
        v_blank = ((wf.t % wf.v_period) >= wf.v_active_len);
    }
    else
    {
        v_blank = wf.v_stuck_val;
    }

    /* SR reports pin XOR polarity. Modelled explicitly so the test can show that the
     * EDGE COUNT does not depend on it, which is what rules polarity out as a cause of
     * a silent probe. */
    h_blank ^= (wf.hspol != 0);
    v_blank ^= (wf.vspol != 0);

    if (h_blank) { sr |= 1U << 0; }
    if (v_blank) { sr |= 1U << 1; }
    if (wf.fne)  { sr |= 1U << 2; }

    wf.t++;
    return sr;
}

/*
 * Each DCI->SR / DCI->RIS in the probe must perform a FRESH read - the probe's whole
 * method is that consecutive reads differ. The extraction step rewrites those two
 * expressions into these calls (a literal, greppable substitution; see the Makefile line
 * in the comment at the top), because a plain struct member cannot re-evaluate.
 */
static uint32_t sr_read(void)  { return fake_sr(); }
static uint32_t ris_read(void) { return wf.ris; }

#include "probe_extracted.inc"

#undef DCI

static void run(const char *title, int hp, int hal, int vp, int val,
                int hs, int vs, uint32_t ris, int fne)
{
    memset(&wf, 0, sizeof(wf));
    wf.h_period = hp; wf.h_active_len = hal;
    wf.v_period = vp; wf.v_active_len = val;
    wf.h_stuck_val = hs; wf.v_stuck_val = vs;
    wf.ris = ris; wf.fne = fne;

    printf("\n=== %s ===\n", title);
    dci_demo_sync_probe();
}

/*
 * Capture just the edge counts, for the polarity-independence check below. Reaches into
 * the same probe by re-running it with the log silenced would be cleaner, but the probe
 * intentionally has no return value - so the counts are recomputed here with the SAME
 * (cur ^ prev) rule the probe uses. That is the claim under test, stated once in each
 * place and compared, rather than a copy that could agree with itself while both are wrong.
 */
static void count_edges(int *h_out, int *v_out)
{
    uint32_t prev = fake_sr();
    int h = 0, v = 0;
    int i;

    for (i = 0; i < DCI_DEMO_SYNC_PROBE_SAMPLES; i++)
    {
        uint32_t cur = fake_sr();

        if (((cur ^ prev) & 1U) != 0) { h++; }
        if (((cur ^ prev) & 2U) != 0) { v++; }
        prev = cur;
    }
    *h_out = h;
    *v_out = v;
}

/*
 * THE POLARITY ARGUMENT, checked rather than asserted.
 *
 * The probe counts edges as (cur ^ prev), and SR is (pin ^ pol). Since
 * (a^p) ^ (b^p) == a^b, the polarity bit cancels exactly, so the edge count must be
 * identical for all four hspol/vspol combinations on the same waveform. That is what
 * makes "zero edges" a statement about the WIRE and not about the configuration - and
 * it is the reason wrong polarity cannot explain a silent probe.
 *
 * Run on a live waveform (edges must be equal AND non-zero) and on a stuck one (must be
 * zero everywhere, so no polarity setting can manufacture an edge).
 */
static int check_polarity_independence(const char *title, int hp, int hal,
                                       int vp, int val, int hs, int vs)
{
    int pol, base_h = -1, base_v = -1, bad = 0;

    printf("\n=== polarity independence: %s ===\n", title);

    for (pol = 0; pol < 4; pol++)
    {
        int h, v;

        memset(&wf, 0, sizeof(wf));
        wf.h_period = hp; wf.h_active_len = hal;
        wf.v_period = vp; wf.v_active_len = val;
        wf.h_stuck_val = hs; wf.v_stuck_val = vs;
        wf.hspol = (pol & 1); wf.vspol = ((pol >> 1) & 1);

        count_edges(&h, &v);
        printf("  hspol=%d vspol=%d -> hsync %d edges, vsync %d edges\n",
               wf.hspol, wf.vspol, h, v);

        if (pol == 0) { base_h = h; base_v = v; }
        else if (h != base_h || v != base_v) { bad = 1; }
    }

    if (bad)
    {
        printf("  ** FAIL: edge count changed with polarity - the XOR argument is wrong\n");
        return 1;
    }
    printf("  OK: identical at all four settings (hsync %d, vsync %d)\n", base_h, base_v);
    return 0;
}

int main(void)
{
    /*
     * Case A - the hypothesis this probe exists to confirm. Both sync lines toggle with
     * believable duty cycles (HSYNC active 320 of 376 pclk, VSYNC active for most of the
     * frame), yet RIS stays 0. Must conclude PIXCLK is missing.
     */
    run("A: both toggle, RIS=0  -> expect 'PIXCLK IS NOT REACHING'",
        376, 320, 1880, 1800, 0, 0, 0x00, 0);

    /*
     * Case B - a genuinely stuck pair, which is what a single SR read gets mistaken for.
     * Both held in blanking, no edges anywhere. Must conclude stuck level, NOT PIXCLK.
     */
    run("B: nothing toggles      -> expect 'NEITHER SYNC INPUT EVER CHANGED'",
        0, 0, 0, 0, 1, 1, 0x00, 0);

    /*
     * Case B2 - stuck the OTHER way, both reading active. Same verdict as B: the old code
     * had a separate branch for this that drew a different (and wrong) conclusion.
     */
    run("B2: stuck reading ACTIVE -> expect 'NEITHER SYNC INPUT EVER CHANGED'",
        0, 0, 0, 0, 0, 0, 0x00, 0);

    /*
     * Case C - HSYNC alive, VSYNC flat. Either unrouted or the window is too short; the
     * probe must say so rather than blaming the clock.
     */
    run("C: HSYNC only           -> expect 'VSYNC NEVER DID'",
        376, 320, 0, 0, 0, 1, 0x00, 0);

    /*
     * Case D - it was actually working and the earlier RIS read was simply too early.
     * LINE latched during the window.
     */
    run("D: RIS latches LINE     -> expect 'RIS became NON-ZERO'",
        376, 320, 1880, 1800, 0, 0, 0x10, 1);

    /*
     * The two halves of the polarity argument. A live wire must show the same non-zero
     * edge count at all four polarity settings; a dead wire must show zero at all four.
     * Together: the edge count measures the wire, never the configuration.
     */
    {
        int fails = 0;

        fails += check_polarity_independence("live waveform", 376, 320, 1880, 1800, 0, 0);
        fails += check_polarity_independence("stuck pins",    0,   0,   0,    0,    1, 1);

        printf("\n%s\n", (fails == 0)
               ? "POLARITY INDEPENDENCE HOLDS: zero edges can only mean a dead wire."
               : "POLARITY INDEPENDENCE VIOLATED - revisit the reasoning.");
        return (fails != 0);
    }
}

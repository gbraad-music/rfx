/*
 * BLEP Table Generator
 * Generates pre-computed BLEP tables for Paula emulation
 *
 * Based on OpenMPT's Paula BLEP implementation (BSD license)
 * Authors: OpenMPT Devs, Antti S. Lankila
 *
 * This tool generates the 5 BLEP tables offline and outputs them as C arrays.
 * Run once to generate fx_paula_blep_tables.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define BLEP_SIZE 2048
#define PAULA_HZ 3546895.0

// Biquad filter for applying Amiga filters to BLEP
typedef struct {
    double b0, b1, b2, a1, a2;
    double x1, x2, y1, y2;
} BiquadFilter;

static void biquad_init(BiquadFilter* f, double b0, double b1, double b2, double a1, double a2) {
    f->b0 = b0; f->b1 = b1; f->b2 = b2;
    f->a1 = a1; f->a2 = a2;
    f->x1 = f->x2 = f->y1 = f->y2 = 0.0;
}

static double biquad_process(BiquadFilter* f, double x0) {
    double y0 = f->b0 * x0 + f->b1 * f->x1 + f->b2 * f->x2 - f->a1 * f->y1 - f->a2 * f->y2;
    f->x2 = f->x1; f->x1 = x0;
    f->y2 = f->y1; f->y1 = y0;
    return y0;
}

// Bessel I0 for Kaiser window
static double izero(double y) {
    double s = 1.0, ds = 1.0, d = 0.0;
    do {
        d = d + 2.0;
        ds = ds * (y * y) / (d * d);
        s = s + ds;
    } while (ds > 1E-7 * s);
    return s;
}

// Generate Kaiser-windowed FIR filter
static void kaiser_fir(double* output, int numTaps, double cutoff, double beta) {
    const double izeroBeta = izero(beta);
    const double kPi = 4.0 * atan(1.0) * cutoff;
    const double xDiv = 1.0 / ((numTaps / 2.0) * (numTaps / 2.0));
    const int numTapsDiv2 = numTaps / 2;

    for (int i = 0; i < numTaps; i++) {
        double fsinc;
        if (i == numTapsDiv2) {
            fsinc = 1.0;
        } else {
            const double x = (double)(i - numTapsDiv2);
            const double xPi = x * kPi;
            fsinc = sin(xPi) * izero(beta * sqrt(1.0 - x * x * xDiv)) / (izeroBeta * xPi);
        }
        output[i] = fsinc * cutoff;
    }
}

// RC lowpass filter
static void make_rc_lowpass(BiquadFilter* f, double sampleRate, double freq) {
    const double omega = (2.0 * M_PI) * freq / sampleRate;
    const double term = 1.0 + 1.0 / omega;
    biquad_init(f, 1.0 / term, 0.0, 0.0, -1.0 + 1.0 / term, 0.0);
}

// Butterworth lowpass
static void make_butterworth(BiquadFilter* f, double sampleRate, double cutoff, double res_dB) {
    // Bilinear z-transform
    const double fc = cutoff;
    const double fs = sampleRate;
    const double res = pow(10.0, (-res_dB / 10.0 / 2.0));

    // Prewarp
    const double wp = 2.0 * fs * tan(M_PI * fc / fs);
    double a0 = 1.0, a1 = 0.0, a2 = 0.0;
    double b0 = 1.0, b1 = sqrt(2.0) * res, b2 = 1.0;

    a2 /= wp * wp;
    a1 /= wp;
    b2 /= wp * wp;
    b1 /= wp;

    const double bd = 4.0 * b2 * fs * fs + 2.0 * b1 * fs + b0;
    biquad_init(f,
        (4.0 * a2 * fs * fs + 2.0 * a1 * fs + a0) / bd,
        (2.0 * a0 - 8.0 * a2 * fs * fs) / bd,
        (4.0 * a2 * fs * fs - 2.0 * a1 * fs + a0) / bd,
        (2.0 * b0 - 8.0 * b2 * fs * fs) / bd,
        (4.0 * b2 * fs * fs - 2.0 * b1 * fs + b0) / bd
    );
}

// Apply biquad filter to entire array
static void apply_filter(BiquadFilter* f, double* data, int size) {
    // Initialize filter to stable state
    for (int i = 0; i < 10000; i++) {
        biquad_process(f, data[0]);
    }

    // Apply filter
    for (int i = 0; i < size; i++) {
        data[i] = biquad_process(f, data[i]);
    }
}

// Integrate FIR to create BLEP
static void integrate(double* data, int size) {
    double total = 0.0;
    for (int i = 0; i < size; i++) {
        total += data[i];
    }

    double startVal = -total;
    for (int i = 0; i < size; i++) {
        startVal += data[i];
        data[i] = startVal;
    }
}

// Quantize and scale to int16
static void quantize(const double* in, int16_t* out, int size) {
    const double cv = 131072.0 / (in[size - 1] - in[0]);  // Scale factor (2^17)

    for (int i = 0; i < size; i++) {
        double val = in[i] * cv;
        out[i] = (int16_t)(-val);  // Negate for BLEP
    }
}

// Write array to file
static void write_table(FILE* f, const char* name, const int16_t* data, int size) {
    fprintf(f, "static const int16_t %s[%d] = {\n", name, size);
    for (int i = 0; i < size; i++) {
        if (i % 8 == 0) fprintf(f, "    ");
        fprintf(f, "%6d", data[i]);
        if (i < size - 1) fprintf(f, ",");
        if (i % 8 == 7) fprintf(f, "\n");
    }
    if (size % 8 != 0) fprintf(f, "\n");
    fprintf(f, "};\n\n");
}

int main(void) {
    printf("Generating BLEP tables for Paula emulation...\n");

    double* tempA500 = (double*)malloc(BLEP_SIZE * sizeof(double));
    double* tempA1200 = (double*)malloc(BLEP_SIZE * sizeof(double));
    int16_t* quantized = (int16_t*)malloc(BLEP_SIZE * sizeof(int16_t));

    BiquadFilter filter;

    // Generate base Kaiser FIR filters
    printf("Generating Kaiser FIR filters...\n");
    kaiser_fir(tempA500, BLEP_SIZE, 21000.0 / PAULA_HZ * 2.0, 8.0);   // A500: beta=8.0
    kaiser_fir(tempA1200, BLEP_SIZE, 21000.0 / PAULA_HZ * 2.0, 9.0);  // A1200: beta=9.0

    // Open output file
    FILE* out = fopen("fx_paula_blep_tables.h", "w");
    if (!out) {
        fprintf(stderr, "Failed to create output file\n");
        return 1;
    }

    fprintf(out, "/*\n");
    fprintf(out, " * Paula BLEP Tables (Auto-generated)\n");
    fprintf(out, " * Based on OpenMPT Paula emulation (BSD license)\n");
    fprintf(out, " */\n\n");
    fprintf(out, "#ifndef FX_PAULA_BLEP_TABLES_H\n");
    fprintf(out, "#define FX_PAULA_BLEP_TABLES_H\n\n");
    fprintf(out, "#include <stdint.h>\n\n");
    fprintf(out, "#define BLEP_SIZE %d\n\n", BLEP_SIZE);

    // A500 LED OFF: 4.9kHz RC filter
    printf("Generating A500 LED OFF table...\n");
    memcpy(tempA500, tempA500, BLEP_SIZE * sizeof(double));
    kaiser_fir(tempA500, BLEP_SIZE, 21000.0 / PAULA_HZ * 2.0, 8.0);
    make_rc_lowpass(&filter, PAULA_HZ, 4900.0);
    apply_filter(&filter, tempA500, BLEP_SIZE);
    integrate(tempA500, BLEP_SIZE);
    quantize(tempA500, quantized, BLEP_SIZE);
    write_table(out, "blep_a500_off", quantized, BLEP_SIZE);

    // A500 LED ON: 4.9kHz RC + 3275Hz Butterworth
    printf("Generating A500 LED ON table...\n");
    kaiser_fir(tempA500, BLEP_SIZE, 21000.0 / PAULA_HZ * 2.0, 8.0);
    make_rc_lowpass(&filter, PAULA_HZ, 4900.0);
    apply_filter(&filter, tempA500, BLEP_SIZE);
    make_butterworth(&filter, PAULA_HZ, 3275.0, -0.70);
    apply_filter(&filter, tempA500, BLEP_SIZE);
    integrate(tempA500, BLEP_SIZE);
    quantize(tempA500, quantized, BLEP_SIZE);
    write_table(out, "blep_a500_on", quantized, BLEP_SIZE);

    // A1200 LED OFF: 32kHz leakage filter
    printf("Generating A1200 LED OFF table...\n");
    kaiser_fir(tempA1200, BLEP_SIZE, 21000.0 / PAULA_HZ * 2.0, 9.0);
    make_rc_lowpass(&filter, PAULA_HZ, 32000.0);
    apply_filter(&filter, tempA1200, BLEP_SIZE);
    integrate(tempA1200, BLEP_SIZE);
    quantize(tempA1200, quantized, BLEP_SIZE);
    write_table(out, "blep_a1200_off", quantized, BLEP_SIZE);

    // A1200 LED ON: 32kHz leakage + 3275Hz Butterworth
    printf("Generating A1200 LED ON table...\n");
    kaiser_fir(tempA1200, BLEP_SIZE, 21000.0 / PAULA_HZ * 2.0, 9.0);
    make_rc_lowpass(&filter, PAULA_HZ, 32000.0);
    apply_filter(&filter, tempA1200, BLEP_SIZE);
    make_butterworth(&filter, PAULA_HZ, 3275.0, -0.70);
    apply_filter(&filter, tempA1200, BLEP_SIZE);
    integrate(tempA1200, BLEP_SIZE);
    quantize(tempA1200, quantized, BLEP_SIZE);
    write_table(out, "blep_a1200_on", quantized, BLEP_SIZE);

    // Unfiltered: Just the Kaiser window
    printf("Generating Unfiltered table...\n");
    kaiser_fir(tempA1200, BLEP_SIZE, 21000.0 / PAULA_HZ * 2.0, 9.0);
    integrate(tempA1200, BLEP_SIZE);
    quantize(tempA1200, quantized, BLEP_SIZE);
    write_table(out, "blep_unfiltered", quantized, BLEP_SIZE);

    fprintf(out, "#endif // FX_PAULA_BLEP_TABLES_H\n");
    fclose(out);

    free(tempA500);
    free(tempA1200);
    free(quantized);

    printf("Done! Generated fx_paula_blep_tables.h\n");
    return 0;
}

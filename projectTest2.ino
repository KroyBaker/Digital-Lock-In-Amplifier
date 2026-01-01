/*
 * A problem I noticed while thinking about the design was the "leakage" of the entire dot product between the reference wave and the input wave. 
 * According to Functional analysis, the dot product between orthogonal waves is zero iff the integral is between an entire period.
 * In order to account for this, I added both a Hanning window and alternated the frequency at which it would show output the count to being multiples of the period
 * Arduino R4 - Digital Lock-In Amplifier (Windowed)
 * Fix: Uses Hanning Window to solve spectral leakage for any frequency.
 */


 //Works for any frequency between 10Hz and 20kHz without changing logic.

#define ARM_MATH_CM4
#define __FPU_PRESENT 1U
#include <arm_math.h> 
#include <FspTimer.h> 

#define SAMPLE_BLOCK_SIZE 1024     //Originally 256
#define SAMPLING_RATE 50000.0f  //
#define INPUT_PIN A0


// Buffers

float32_t inputBuffer[SAMPLE_BLOCK_SIZE]; 
float32_t refSin[SAMPLE_BLOCK_SIZE];
float32_t refCos[SAMPLE_BLOCK_SIZE];
float32_t window[SAMPLE_BLOCK_SIZE]; // NEW: Windowing Array
volatile uint16_t rawAdcBuffer[SAMPLE_BLOCK_SIZE];

// Logic Variables
float currentFreq = 1000.0f;
float32_t totalMagnitude = 0.0f;
uint32_t blockCount = 0;

// Timer Variables
FspTimer samplingTimer;
volatile bool dmaBufferFull = false;
volatile int bufferIndex = 0;

void setTargetFrequency(float freq) {
    currentFreq = freq; //Hmmm, maybe weird practice. We'll see
    for(int i = 0; i < SAMPLE_BLOCK_SIZE; i++) {
        float32_t phase = (2.0f * PI * currentFreq * i) / SAMPLING_RATE;
        refSin[i] = arm_sin_f32(phase);
        refCos[i] = arm_cos_f32(phase);
    }
    // Reset Averaging
    totalMagnitude = 0; 
    blockCount = 0; 
}

// ISR 
void timer_callback(timer_callback_args_t* p) {
    if (bufferIndex < SAMPLE_BLOCK_SIZE) {
        rawAdcBuffer[bufferIndex] = analogRead(INPUT_PIN); //Where we read stuff
        bufferIndex++;
    } else {
        dmaBufferFull = true;
    }
}

void setupTimer() {
    uint8_t timer_type = GPT_TIMER;
    int8_t t_index = FspTimer::get_available_timer(timer_type);
    if (t_index < 0) t_index = FspTimer::get_available_timer(timer_type, true); 

    if (t_index != -1) {
        samplingTimer.begin(TIMER_MODE_PERIODIC, timer_type, t_index, SAMPLING_RATE, 50.0f, timer_callback);
        samplingTimer.setup_overflow_irq();
        samplingTimer.open();
        samplingTimer.start();
    }
}

void setup() {
    Serial.begin(115200);
    analogReadResolution(14);
    
    // 1. Generate Hanning Window (Run once)
    // Formula: 0.5 * (1 - cos(2*PI*i / (N-1)))
    for (int i = 0; i < SAMPLE_BLOCK_SIZE; i++) {
        window[i] = 0.5f * (1.0f - arm_cos_f32(2.0f * PI * i / (SAMPLE_BLOCK_SIZE - 1)));
    }

    setTargetFrequency(1000.0f);
    setupTimer();
}

void loop() {
    if (dmaBufferFull) {
        float32_t sumI, sumQ, mag;

        // 1. Conditioning + Windowing
        for(int i=0; i<SAMPLE_BLOCK_SIZE; i++) {
            // Normalize (-1.0 to 1.0)
            float32_t val = ((float32_t)rawAdcBuffer[i] - 8191.5f) / 8192.0f;
            
            // APPLY WINDOW: This fixes the leakage/orthogonality error!
            inputBuffer[i] = val * window[i]; 
        }

        // 2. The Orthogonality Check
        arm_dot_prod_f32(inputBuffer, refSin, SAMPLE_BLOCK_SIZE, &sumI);
        arm_dot_prod_f32(inputBuffer, refCos, SAMPLE_BLOCK_SIZE, &sumQ);
        
        // Normalization (Windowing reduces power by half, so we adjust)
        float32_t I = sumI / (SAMPLE_BLOCK_SIZE / 4.0f); // Factor 4.0 for Hanning
        float32_t Q = sumQ / (SAMPLE_BLOCK_SIZE / 4.0f);
        arm_sqrt_f32((I*I) + (Q*Q), &mag);

        // 3. Simple Averaging for Noise Reduction
        totalMagnitude += mag;
        blockCount++;
        
        // Print every ~0.5 seconds (approx 80 blocks)
        if (blockCount >= 80) {
            //Serial.print("Freq: ");
            //Serial.print(currentFreq);
            //Serial.print(" Hz | Mag: ");
            //Serial.println(totalMagnitude / blockCount, 6);
            //Not yet printing as it will mess up timing
            
            totalMagnitude = 0; 
            blockCount = 0; 
        }

        bufferIndex = 0; 
        dmaBufferFull = false;
    }
}

//I decided to use the custom ARM arduino sine and cosine functions as they are optimizeed for the microcontroller.
//The Arduino itself will not be the final PCB I will be using, however, it will be good as a prototype to build the code.


#define ARM_MATH_CM4  // Tell the compiler we are on a Cortex-M4
#define __FPU_PRESENT 1U // Tells the library to use the Hardware FPU
#include <arm_math.h>
#include <FspTimer.h> // Native R4 Timer Library

// --- Configuration ---
#define SAMPLE_BLOCK_SIZE 1024 //Thinking of increasing a lot
#define SAMPLING_RATE 40000.0f  // 40kHz sampling rate
#define TARGET_FREQ 1000.0f     // The frequency we want to detect (1kHz)

// --- Buffers ---
// CMSIS-DSP requires float32_t arrays
float32_t inputBuffer[SAMPLE_BLOCK_SIZE]; 
float32_t refSin[SAMPLE_BLOCK_SIZE];
float32_t refCos[SAMPLE_BLOCK_SIZE];


float32_t totalMagnitude;
unsigned int totalMagnitudeSamples; //Will be all the total Magnitudes we sampled




// Raw buffer for the Interrupt Service Routine (ISR) to fill
volatile uint16_t rawAdcBuffer[SAMPLE_BLOCK_SIZE];

// --- Global Variables for Synchronization ---
FspTimer samplingTimer;
volatile bool dmaBufferFull = false;
volatile int bufferIndex = 0;

// --- Interrupt Service Routine (ISR) ---
// This function runs automatically 40,000 times per second
void timer_callback(timer_callback_args_t __attribute((unused)) *p_args) {
    if (bufferIndex < SAMPLE_BLOCK_SIZE) {
        // Read ADC directly and store. 
        // This is fast enough to happen in the background.
        rawAdcBuffer[bufferIndex] = analogRead(A0);
        bufferIndex++;
    } else {
        // Buffer is full. Signal the main loop to process.
        // We stop collecting (ignore timer ticks) until the loop resets us.
        dmaBufferFull = true; 
    }
}

// --- Setup Timer Helper ---
void setupTimer() {
    uint8_t timer_type = GPT_TIMER;
    int8_t t_index = FspTimer::get_available_timer(timer_type);
    
    if (t_index < 0) {
        t_index = FspTimer::get_available_timer(timer_type, true); // Force a timer if none free
    }

    if (t_index != -1) {
       samplingTimer.begin(
            TIMER_MODE_PERIODIC,    // 1. Mode
            timer_type,             // 2. Type (GPT)
            t_index,                // 3. Channel
            SAMPLING_RATE,          // 4. Frequency (Hz)
            50.0f,                  // 5. Duty Cycle (50%)
            timer_callback          // 6. Callback Function
        );
        
        samplingTimer.setup_overflow_irq();
        samplingTimer.open();
        samplingTimer.start();

    } else {
        Serial.println("Failed to initialize Timer!");
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial); // Wait for Serial Monitor

    Serial.println("Initializing Digital Lock-in Amplifier...");

    // 1. Initialize Reference Waves (Pre-compute once for speed)
    // We generate the "Perfect" sine/cos waves to compare against
    for(int i = 0; i < SAMPLE_BLOCK_SIZE; i++) {
        // Calculate phase for this specific sample time
        float32_t phase = (2.0f * PI * TARGET_FREQ * i) / SAMPLING_RATE;
        
        // Use CMSIS functions to fill the reference tables
        refSin[i] = arm_sin_f32(phase);
        refCos[i] = arm_cos_f32(phase);
    }

    // 2. Configure ADC
    analogReadResolution(14); // Set R4 to 14-bit mode (0-16383)

    // 3. Start the Hardware Timer
    setupTimer();
    
    Serial.println("System Running. Outputting Magnitude...");
}

void loop() {
    // Wait for the ISR to fill the buffer
    if (dmaBufferFull) {
        
        float32_t sumI, sumQ;
        float32_t magnitude;

        // 4. Signal Conditioning
        // Convert integer ADC (0-16383) to centered float (-1.0 to 1.0)
        // Midpoint is 8191.5
        for(int i=0; i<SAMPLE_BLOCK_SIZE; i++) {
            inputBuffer[i] = ((float32_t)rawAdcBuffer[i] - 8191.5f) / 8192.0f;
        }

        // 5. THE LOCK-IN MATH (CMSIS-DSP Optimization)
        // Calculate Dot Products (Mixing + Averaging Sum)
        // In-Phase Component (I)
        arm_dot_prod_f32(inputBuffer, refSin, SAMPLE_BLOCK_SIZE, &sumI);
        // Quadrature Component (Q)
        arm_dot_prod_f32(inputBuffer, refCos, SAMPLE_BLOCK_SIZE, &sumQ);

        // 6. Normalize and Calculate Magnitude
        // We divide by (N/2) because dot product sums N samples
        float32_t I = sumI / (SAMPLE_BLOCK_SIZE / 2.0f);
        float32_t Q = sumQ / (SAMPLE_BLOCK_SIZE / 2.0f);
        
        // Calculate Magnitude = sqrt(I^2 + Q^2)
        arm_sqrt_f32((I*I) + (Q*Q), &magnitude);

        // 7. Output Result
        // Use Serial Plotter (Ctrl+Shift+L) to view this as a graph
        Serial.println(magnitude, 6);

        // 8. Restart Data Collection
        // We must reset the index so the ISR starts filling from 0 again
        bufferIndex = 0; 
        dmaBufferFull = false; 
    }
}

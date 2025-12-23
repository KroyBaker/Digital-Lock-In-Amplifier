#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

// --- User Provided Constants & Macros ---
#define PI 3.141592653f
#define piOverTwo 1.57079632679f
#define sineConstant1 0.9174f     // Lower bound for approximation
#define sineConstant2 2.224192f   // Upper bound for approximation
#define INV6 0.1666666667f
#define INV120 0.0083333333f
#define TWO_PI (2.0f * PI)


//Attempting to Create my own Custom Sine Function.
float sineFunc(float x){ 
      float ans;
      while(x > 2PI){
        x = x - 2PI;
      }
      if(x < sineConstant1){
        return x;
      }
      else if(x < sineConstant2){
        ans = 1 + ((x-piOverTwo)(x-piOverTwo)/2)(-1 + (x-piOverTwo)*(x-piOverTwo)/12);
        return ans;
      }
      else if(x<PI){
          return (-x+PI);
      }
      else{
          return -sineFunc(x-PI);
      }
}

float cosFunc(float x) {
    return sineFunc(x + piOverTwo);
}

     


/******************************************************************************


                              Online C++ Compiler.

               Code, Compile, Run and Debug C++ program online.

Write your code in this editor and press "Run" button to compile and execute it.


*******************************************************************************/


#include <iostream>

#define PI 3.141592653


#define piOverTwo 1.57079632679 


#define sineConstant1 0.9174 //Used as lower bound for approximation


#define sineConstant2 2.224192 //Used for upperbound for approximation

#define INV6 0.1666666667f

#define INV120 0.0083333333f


float sineFunc(float x){ 

        float ans;

        int sign = 1;

          // Fast range reduction to [0, 2π)

        x -= (int)(x * (1.0f / (2*PI))) * (2*PI);

          

        if (x > PI){

            x = 2*PI - x;

            sign = -1;

        }


        //Horner Form

        if(x <= sineConstant1){

              float x2 = x*x;

              return sign * x * (1 - x2*(INV6- x2*(INV120)));

        }


        else if(x < sineConstant2){


          float d = x - piOverTwo;

          ans = 1.0f + (d*d)*(-0.5f + (d*d)*(INV120*10));

          return sign * ans;

        }

        

      float d = x - PI;

      float d2 = d * d;

      return sign*(d * (-1.0f + d2 * (INV6 - d2*(INV120))));

  }

  

  

int main()

{

    std::cout<<"Hello World";


    return 0;

}



By using the following info already given, create a square wave function, and add a program which takes the fourier transform of that square wave, and gives back a vector with the given results

Here is the complete C++ solution. It includes your optimized sine approximation, adds a derived cosine function, generates a square wave, and implements a Discrete Fourier Transform (DFT) to analyze the frequencies.
Key Features of this Implementation:

    cosFunc: Derived simply as sineFunc(x + PI/2) to reuse your approximation logic.

    generateSquareWave: Creates a buffer of samples toggling between 1 and −1.

    computeDFT: Calculates the Discrete Fourier Transform manually using the custom trig functions. It returns a vector of magnitudes representing the frequency spectrum.

C++

/******************************************************************************
             Optimized Sine/Cosine Approximation & Fourier Transform
*******************************************************************************/

#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

// --- User Provided Constants & Macros ---
#define PI 3.141592653f
#define piOverTwo 1.57079632679f
#define sineConstant1 0.9174f     // Lower bound for approximation
#define sineConstant2 2.224192f   // Upper bound for approximation
#define INV6 0.1666666667f
#define INV120 0.0083333333f
#define TWO_PI (2.0f * PI)

// --- User Provided Sine Function ---
float sineFunc(float x) {
    float ans;
    int sign = 1;

    // Fast range reduction to [0, 2PI)
    // Note: Assuming positive inputs for simplicity in this demo context
    x -= (int)(x * (1.0f / TWO_PI)) * TWO_PI;

    if (x > PI) {
        x = TWO_PI - x;
        sign = -1;
    }

    // Horner Form Expansion
    if (x <= sineConstant1) {
        float x2 = x * x;
        return sign * x * (1 - x2 * (INV6 - x2 * (INV120)));
    } else if (x < sineConstant2) {
        float d = x - piOverTwo;
        // Note: Using user provided coefficients
        ans = 1.0f + (d * d) * (-0.5f + (d * d) * (INV120 * 10));
        return sign * ans;
    }

    float d = x - PI;
    float d2 = d * d;
    return sign * (d * (-1.0f + d2 * (INV6 - d2 * (INV120))));
}

// --- Helper: Cosine derived from Sine ---
// cos(x) = sin(x + pi/2)
float cosFunc(float x) {
    return sineFunc(x + piOverTwo);
}

// --- 1. Square Wave Generator ---
// Generates a vector of samples for a square wave
std::vector<float> generateSquareWave(float frequency, float sampleRate, int numSamples) {
    std::vector<float> wave;
    wave.reserve(numSamples);

    for (int n = 0; n < numSamples; n++) {
        float t = (float)n / sampleRate;
        // Standard Square Wave: sgn(sin(2*pi*f*t))
        // Using our custom sineFunc
        float val = sineFunc(TWO_PI * frequency * t);
        
        // Push 1.0 or -1.0
        wave.push_back(val >= 0.0f ? 1.0f : -1.0f);
    }
    return wave;
}

// --- 2. Discrete Fourier Transform (DFT) ---
// Takes a signal time-series and returns the Magnitude Spectrum vector
std::vector<float> computeDFT(const std::vector<float>& signal) {
    int N = signal.size();
    std::vector<float> magnitudes;
    
    // We only need to calculate up to N/2 (Nyquist limit)
    for (int k = 0; k < N / 2; k++) {
        float realPart = 0.0f;
        float imagPart = 0.0f;

        for (int n = 0; n < N; n++) {
            // Euler's Formula: e^(-i*theta) = cos(theta) - i*sin(theta)
            // theta = 2*pi*k*n / N
            float theta = TWO_PI * k * n / N;
            
            float cosTerm = cosFunc(theta);
            float sinTerm = sineFunc(theta);

            realPart += signal[n] * cosTerm;
            imagPart -= signal[n] * sinTerm;
        }

        // Magnitude = sqrt(Re^2 + Im^2)
        // Normalized by N/2 for easier reading (except DC component)
        float mag = std::sqrt(realPart * realPart + imagPart * imagPart);
        if (k != 0) mag *= (2.0f / N); 
        else mag *= (1.0f / N);

        magnitudes.push_back(mag);
    }
    return magnitudes;
}


int main(void){
//Later editions

}


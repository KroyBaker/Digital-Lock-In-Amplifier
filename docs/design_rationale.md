December 29th, 2025

Project Objective: I am designing a research-grade Digital Lock In Amplifier project optimized specifically for Quantum Photonics measurements (specifically targeting hBN emitter characterization).
The most important parts of the design will creating a low-noise, high-speed alternative to commercial units 
by leveraging a heterogeneous architecture.

As of right now, I am currently designing the footprints and schematics for the PCB, as well as gathering a design which will be cheap enough to be attainable 
on a student budget, while also precise enough to be used for my future research goals.



December 30th, 2025

I decided to use the LTC2208 as my Analog Digital Converter (ADC). Its 130 Msps sampling rate and 100 dB SFDR (Spurious-Free Dynamic Range) are ideal for the DLIA, which must resolve signals 60 dB to 80 dB (1,000x to 10,000x) below the noise floor

For the ADC, we still need a way to make sure it only picks up specific frequencys. This is where the Analog Front-End (AFE) is used.

The AFE for this design comprises a Transimpedance Amplifier (TIA), an anti-aliasing filter, and a differential ADC driver. The TIA first converts the input signal current into a proportional voltage. This voltage is then processed by a 4th-order Butterworth low-pass filter. I selected the Butterworth topology for its maximally flat passband response, which ensures signal integrity, while the 4th-order design provides a sweet spot, offering a steep enough roll-off to suppress noise without introducing excessive group delay.
With a system sampling rate of 130 Msps, the Nyquist limit is 65 MHz. However, I decided to set the filter’s cutoff frequency at 50 MHz to provide a sufficient transition band; this ensures that frequencies near the Nyquist limit are adequately attenuated before sampling. Finally, the signal is passed through a 50 Ω matching resistor to the differential ADC driver, which prepares the signal for the LTC2208 by providing the required balanced format and common-mode biasing.

Designing the filter is easy, the difficult part is choosing which exact resistors, capacitors, and inductors to use.
Currently, I am aiming to find extremely precise and sturdy parts which will be able to work well under a fair amount of heat.

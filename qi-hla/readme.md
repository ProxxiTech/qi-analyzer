
# WPC Qi Protocol Decoder
## High Level Analyzer for Saleae Logic 2

The latest version of this analyzer is available on Github: [Qi-HLA](https://github.com/ProxxiTech/qi-analyzer/qi-hla).

### IMPORTANT - Low Level Analyzer
This High Level Analyzer decodes Qi packets from raw bytes, and must be paired with the [Qi Low Level Analyzer](https://github.com/ProxxiTech/qi-analyzer), which generates the raw bytes from a channel of Qi protocol data.


# Getting Started

## Setup

* Download the binaries for the latest version of Qi-Analyzer from the repository's [Releases](https://github.com/ProxxiTech/qi-analyzer/releases).
* Extract the downloaded zip file to a directory.
* In Logic 2, go to Preferences and set the Custom Low Level Analyzers directory to the directory where you extracted the downloaded zip file. See [Importing a Custom Low Level Analyzer](https://support.saleae.com/faq/technical-faq/setting-up-developer-directory) for more information. Note that additional steps may be required for MacOS users, so please follow the directions in the Saleae FAQ.
* Install the latest version of Qi-HLA from Logic 2 > Extensions > Load Existing Extension... and select `<qi-analyzer>/qi-hla/extension.json` from the unziped download. The HLA is also published on the Saleae extension marketplace as `WPC Qi HLA`, but there's a chance that it won't match the version of the LLA you donwloaded.
* Build the [circuit](#circuit) from Freescale AN4701.
* Connect both sides of the Tx coil to the circuits inputs (leave them connected to the Qi circuit though, of course!)
* Connect the output of the circuit to a channel on your Saleae logic analyzer.
* In Logic 2, add the `WPC Qi LLA` analyzer to the channel and optionally disable `Show in protocol results table`.
* Add `WPC Qi HLA` and set its input analyzer as the `WPC Qi LLA` that was added previously. Leave `Show in protocol results table` enabled.

### Recommended capture settings

* Sampling rate: 1MS/S
* Voltage level: 3.3+ Volts
* Do not use the Glitch filter. The low-level analyzer has glitch filtering built in and the Logic 2's glitch filter will interfere (it's essentially a low-pass filter, which messes up the timing of the edges).


## Circuit

The Tx coil voltage must be filtered to extract the Qi communication packets using a circuit such as the one described in [Freescale AN4701](https://www.nxp.com/docs/en/application-note/AN4701.pdf), and the output of that circuit should be used as the input channel to the Qi Analyzer.

### Tips for AN4701

* Don't forget the decoupling caps on the opamps!
* Change R713 from 18k to 1.5k to increase the peak-to-peak range, which helps a lot with generating valid data.
* Put a 1M resistor in R722 (normally NC) to add some hysteresis to the comparitor.
* Have a spacer of 1-2mm between the Tx and Rx coils.

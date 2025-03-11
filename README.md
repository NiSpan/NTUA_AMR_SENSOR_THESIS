# Steel Health Monitoring with AMR Sensors

## Abstract
Steel health monitoring is of vital importance for assessing the integrity of steel structures in critical infrastructure and industrial applications. Magnetic techniques, such as monitoring differential permeability, which can be used in combination with Magnetic Stress Calibration (MASC) curves, enable non-destructive testing to detect defects in ferromagnetic materials like steel by monitoring their magnetic properties.

In this diploma thesis, an Internet of Things (IoT) sensor is implemented, consisting of an anisotropic magnetoresistance (AMR) sensor and a microcontroller, for measuring the surface magnetic flux density and wirelessly transmitting data. The goal is to enable remote real-time steel health monitoring through a web-based graphical interface. Each sensor can operate autonomously or establish a wireless mesh network consisting of identical sensor nodes, allowing deployment in hard-to-reach areas.

The sensor system was evaluated for its ability to detect magnetic field variations in steel specimens. Finally, a wireless mesh network of sensors was implemented and tested on a steel sample, transmitting its measurements to a central database, enabling real-time visualization of the collected data.

## Features
- Non-destructive testing for steel health monitoring
- AMR sensor for magnetic flux density measurements
- IoT-enabled wireless data transmission
- Support for Wi-Fi mesh networking
- Real-time data visualization via a web interface

## Keywords
- Anisotropic Magnetoresistance (AMR) Sensors
- Steel Health Monitoring
- Magnetic Stress Calibration (MASC)
- Internet of Things (IoT)
- Wi-Fi Mesh Network

## Installation
1. Clone this repository:
   ```sh
   git clone https://github.com/yourusername/steel-health-monitoring.git
   ```
2. Navigate to the project directory:
   ```sh
   cd steel-health-monitoring
   ```
3. Install dependencies (if applicable):
   ```sh
   pip install -r requirements.txt  # For Python projects
   ```

## Usage
1. Deploy the sensor nodes in the desired locations.
2. Power on the microcontrollers and establish a Wi-Fi mesh network.
3. Access the web interface to visualize real-time sensor data.

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author
[Your Name] - [Your Institution]

## Acknowledgments
- National Technical University of Athens
- Supervisors and research advisors

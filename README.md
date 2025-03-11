# Steel Health Monitoring with AMR Sensors

## Description
This repository contains code used for the implementation of an IoT-enabled AMR sensor, using:
- LIS2MDL AMR sensor for reading magnetic flux density 
- ESP32-C6 as the microcontroller of the embedded system
- ESP-IDF for development
- ESP-WIFI-MESH for mesh network implementation

## Abstract
Steel health monitoring is of vital importance for assessing the integrity of steel structures in critical infrastructure and industrial applications. Magnetic techniques, such as monitoring differential permeability, which can be used in combination with Magnetic Stress Calibration (MASC) curves, enable non-destructive testing to detect defects in ferromagnetic materials like steel by monitoring their magnetic properties.

In this diploma thesis, an Internet of Things (IoT) sensor is implemented, consisting of an anisotropic magnetoresistance (AMR) sensor and a microcontroller, for measuring the surface magnetic flux density and wirelessly transmitting data. The goal is to enable remote real-time steel health monitoring through a web-based graphical interface. Each sensor can operate autonomously or establish a wireless mesh network consisting of identical sensor nodes, allowing deployment in hard-to-reach areas.

The sensor system was evaluated for its ability to detect magnetic field variations in steel specimens. Finally, a wireless mesh network of sensors was implemented and tested on a steel sample, transmitting its measurements to a central database, enabling real-time visualization of the collected data.

## Features
- Non-destructive testing for steel health monitoring
- AMR sensor for magnetic flux density measurements
- IoT-enabled wireless data transmission
- Support for Wi-Fi mesh networking

## Keywords
- Anisotropic Magnetoresistance (AMR) Sensors
- Steel Health Monitoring
- Magnetic Stress Calibration (MASC)
- Internet of Things (IoT)
- Wi-Fi Mesh Network

## Author
Nikolaos Spanidis
School of Electrical and Comupter Engineering
National Technical University of Athens

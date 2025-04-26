# 5780-mini-project

### Members
Wei Siew Liew

William Carr

Colin Liechty

Trae Hillstead


# Inverted pendulum on a spinning disk

## About

The main goal of this project was to use a motor that spins a disk to balance a stick on the end of it.

## Description

There is a 3D printed disk attached to a DC motor mounted on a block of wood. At the end of the disk is a stick which is attached to a long axel going through the disk. The stick is also attached to a gear that is in a 90 degree pitch of another gear. The second gear is a freely spinning gear with a magnet on the end that hovers over a magnetic encoder. Also on the disk, there is a bread board that contains an STM32, a power supply, and a Bluetooth module. Additonaly there is another STM32 that is connected to the recieving Bluetooth module and drives the motor driver.

## How it works

When the stick falls it rotates the horizonal gear. The horizonal gear spins a magnet which a magnetic encoder reads. The readings from the magnetic encoder are transmitted to the STM32 through I2C. After configuring the zero position of the magnetic encoder, the microcontroller performs a control feedback loop to determine the PWM duty cycle and motor direction. 

The values for PWM duty cycle and motor direction are encoded as an 8-bit command and sent via USART to the Buetooth module for transmission to the motor controller board. The 8-bit command has the following format: command[6:0] encode the PWM duty cycle (0 to 100) and command[7] encodes the motor direction. The motor controller board updates the PWM duty cycle and motor direction whenever an interrupt is triggered by USART1, indicating that a new command has been received.

## Photos
![IMG_5142](https://github.com/user-attachments/assets/d7c52bec-53e8-4729-8df6-f70c4dea501a)

The whole set up

![IMG_5143](https://github.com/user-attachments/assets/85b958da-5574-485e-a68e-f8bc7f28ab11)

One of the Bluetooth modules

![IMG_5146](https://github.com/user-attachments/assets/c05ac081-2b94-47ca-b10d-0eb0e6dc9308)

The magnetic encoder sitting under the horizontal gear

## Hardware

| Component            | Description                        |
|----------------------|------------------------------------|
| Microcontroller x 2  | STM32                              |
| Motor Driver         | DRV8871                            |
| Motors               | DC motor / Stepper                 |
| Sensors              | Magnetic Encoder                   |
|Communication protocol| Bluetooth modules                  |
| Other                | 3D printed parts, base, connectors |

# Software

The source code for the project can be found in the Src directory and the header files can be found 
in the Inc directory. The important source files are described in the following table:

| File                 | Description                        |
|----------------------|------------------------------------|
| project_main.c       | Code for configuring the microcontroller boards and running the main loops.                   |
| motor.c              | Functions for configuring timer peripheral for motor PWM and controlling motor direction.     |
| magnetic_encoder.c   | Functions for interacting with the I2C peripheral, used for accessing the magnetic encoder.   |
| usart.c              | Functions for interacting with the USART peripheral, used for bluetooth communication.        |

Using platformio, the executable binary for the microcontroller that reads the magnetic encoder and performs the computations for the motor commands can be built by specifying the `project_main_send` env. The executable binary for the microcontroller that decodes the motor command and controls the motors can be built by specifying the `project_main_recv` env.


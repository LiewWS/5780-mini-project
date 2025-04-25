# 5780-mini-project

### Members
Wei Siew Liew

William Carr

Colin Liechty

Trae Hillstead


# Inverted pendulum on a spinning disk

## About

The main goal of this project was to use a motor that spun a disk to balance a stick on the end of it.

## Description

There is a 3D disk attached to a DC motor mounted on a block of wood. At the end of the disk is a stick which is attached to a long axel going through the disk. The stick is also attached to a gear that is in a 90 degree pitch of another gear. The second gear is a freely spinning gear with a magnet on the end that hovers over a magnetic encoder. Also on the disk, there is a bread board that contains an STM32, a power supply, and a Bluetooth module. Additonaly there is another STM32 that is connected to the recieving Bluetooth module and drives the motor driver.

## How it works

When the stick falls it rotates the horizonal gear. The horizonal gear spins a magnet which a magnetic encoder reads. The readings from the magnetic encoder are transmitted to the STM32 through I2C. The microcontroller performs a control feedback loop to determine a PWM. The PWM is then converted to an 8 bit usart byte for the master bluetooth module to send to the the slave Bluetooth. The 8 bit byte contains 7 bits for the PWM 1 - 100. and the 8th MSB to determine direction. The slave bluetooth recieves the PWM information and connects to another STM32 which drives the motor. 

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
|Communication protocol| Bluetooth modules
| Other                | 3D printed parts, base, connectors |


[ control info ]
# control_cycle = 8   # milliseconds # < -- Original
control_cycle = 10   # milliseconds

[ port info ]
# PORT NAME  | BAUDRATE | DEFAULT JOINT
/dev/ttyUSB0 | 1000000  | l_arm_sh_p1
/dev/ttyUSB1 | 1000000  | r_arm_sh_p1
/dev/ttyUSB2 | 1000000  | l_arm_wr_r
/dev/ttyUSB3 | 1000000  | r_arm_wr_r

#/dev/ttyUSB2 | 2000000  | l_arm_sh_p1 #Broken!
#/dev/ttyUSB3 | 2000000  | r_leg_hip_y #Broken!
    
[ device info ]
# TYPE    | PORT NAME    | ID  | MODEL          | PROTOCOL | DEV NAME     | BULK READ ITEMS
dynamixel | /dev/ttyUSB1 | 1   | H54-100-S500-R | 2.0      | r_arm_sh_p1  | present_position, present_velocity, present_current
dynamixel | /dev/ttyUSB0 | 2   | H54-100-S500-R | 2.0      | l_arm_sh_p1  | present_position, present_velocity, present_current
dynamixel | /dev/ttyUSB1 | 3   | H54-100-S500-R | 2.0      | r_arm_sh_r   | present_position, present_velocity, present_current
dynamixel | /dev/ttyUSB0 | 4   | H54-100-S500-R | 2.0      | l_arm_sh_r   | present_position, present_velocity, present_current
dynamixel | /dev/ttyUSB1 | 5   | H54-100-S500-R | 2.0      | r_arm_sh_p2  | present_position, present_velocity, present_current
dynamixel | /dev/ttyUSB0 | 6   | H54-100-S500-R | 2.0      | l_arm_sh_p2  | present_position, present_velocity, present_current
dynamixel | /dev/ttyUSB1 | 7   | H54-100-S500-R | 2.0      | r_arm_el_y   | present_position, present_velocity, present_current
dynamixel | /dev/ttyUSB0 | 8   | H54-100-S500-R | 2.0      | l_arm_el_y   | present_position, present_velocity, present_current
dynamixel | /dev/ttyUSB3 | 9   | Seed-67        | 1.0      | r_arm_wr_r   | present_position
dynamixel | /dev/ttyUSB2 | 10  | Seed-67        | 1.0      | l_arm_wr_r   | present_position
dynamixel | /dev/ttyUSB3 | 11  | Seed-67-Y      | 1.0      | r_arm_wr_y   | present_position
dynamixel | /dev/ttyUSB2 | 12  | Seed-67-Y      | 1.0      | l_arm_wr_y   | present_position
dynamixel | /dev/ttyUSB3 | 13  | Seed-67-P      | 1.0      | r_arm_wr_p   | present_position
dynamixel | /dev/ttyUSB2 | 14  | Seed-67-P      | 1.0      | l_arm_wr_p   | present_position
dynamixel | /dev/ttyUSB1 | 27  | H54-100-S500-R | 2.0      | torso_y      | present_position, present_velocity, present_current
dynamixel | /dev/ttyUSB0 | 28  | H42-20-S300-R  | 2.0      | head_y       | present_position, present_velocity, present_current
dynamixel | /dev/ttyUSB0 | 29  | H42-20-S300-R  | 2.0      | head_p       | present_position, present_velocity, present_current

################################################################################################################
## Seed-Robotics
#sensor    | /dev/ttyUSB4 | 101 | SEED-EROSBRD   | 1.0      | r_arm_board        | model_number
#sensor    | /dev/ttyUSB4 | 102 | SEED-EROSBRD   | 1.0      | l_arm_board        | model_number

dynamixel | /dev/ttyUSB3 | 103 | Seed-56        | 1.0      | r_arm_thumb_y      | present_position
dynamixel | /dev/ttyUSB2 | 104 | Seed-56        | 1.0      | l_arm_thumb_y      | present_position
dynamixel | /dev/ttyUSB3 | 105 | Seed-56        | 1.0      | r_arm_thumb_p      | present_position
dynamixel | /dev/ttyUSB2 | 106 | Seed-56        | 1.0      | l_arm_thumb_p      | present_position
dynamixel | /dev/ttyUSB3 | 107 | Seed-56        | 1.0      | r_arm_index_p      | present_position
dynamixel | /dev/ttyUSB2 | 108 | Seed-56        | 1.0      | l_arm_index_p      | present_position
dynamixel | /dev/ttyUSB3 | 109 | Seed-56        | 1.0      | r_arm_middle_p     | present_position
dynamixel | /dev/ttyUSB2 | 110 | Seed-56        | 1.0      | l_arm_middle_p     | present_position
dynamixel | /dev/ttyUSB3 | 111 | Seed-56        | 1.0      | r_arm_finger45_p   | present_position
dynamixel | /dev/ttyUSB2 | 112 | Seed-56        | 1.0      | l_arm_finger45_p   | present_position

################################################################################################################
## Robot Original Arms

#dynamixel | /dev/ttyUSB0 | 9   | H42-20-S300-R  | 2.0      | r_arm_wr_r   | present_position, present_voltage
#dynamixel | /dev/ttyUSB3 | 10  | H42-20-S300-R  | 2.0      | l_arm_wr_r   | present_position, present_voltage
#dynamixel | /dev/ttyUSB0 | 11  | H42-20-S300-R  | 2.0      | r_arm_wr_y   | present_position, present_voltage
#dynamixel | /dev/ttyUSB2 | 12  | H42-20-S300-R  | 2.0      | l_arm_wr_y   | present_position, present_voltage
#dynamixel | /dev/ttyUSB0 | 13  | H42-20-S300-R  | 2.0      | r_arm_wr_p   | present_position, present_voltage
#dynamixel | /dev/ttyUSB2 | 14  | H42-20-S300-R  | 2.0      | l_arm_wr_p   | present_position, present_voltage

################################################################################################################
## Robot Legs

#dynamixel | /dev/ttyUSB2 | 15  | H54-100-S500-R | 2.0      | r_leg_hip_y  | present_position, present_voltage
#dynamixel | /dev/ttyUSB3 | 16  | H54-100-S500-R | 2.0      | l_leg_hip_y  | present_position, present_voltage
#dynamixel | /dev/ttyUSB2 | 17  | H54-200-S500-R | 2.0      | r_leg_hip_r  | present_position, present_voltage
#dynamixel | /dev/ttyUSB3 | 18  | H54-200-S500-R | 2.0      | l_leg_hip_r  | present_position, present_voltage
#dynamixel | /dev/ttyUSB2 | 19  | H54-200-B500-R | 2.0      | r_leg_hip_p  | present_position, present_voltage
#dynamixel | /dev/ttyUSB3 | 20  | H54-200-B500-R | 2.0      | l_leg_hip_p  | present_position, present_voltage
#dynamixel | /dev/ttyUSB2 | 21  | H54-200-S500-R | 2.0      | r_leg_kn_p   | present_position, present_voltage
#dynamixel | /dev/ttyUSB3 | 22  | H54-200-S500-R | 2.0      | l_leg_kn_p   | present_position, present_voltage
#dynamixel | /dev/ttyUSB2 | 23  | H54-200-B500-R | 2.0      | r_leg_an_p   | present_position, present_voltage, external_port_data_1, external_port_data_2
#dynamixel | /dev/ttyUSB3 | 24  | H54-200-B500-R | 2.0      | l_leg_an_p   | present_position, present_voltage, external_port_data_1, external_port_data_2
#dynamixel | /dev/ttyUSB2 | 25  | H54-200-S500-R | 2.0      | r_leg_an_r   | present_position, present_voltage, external_port_data_1, external_port_data_2, external_port_data_3, external_port_data_4
#dynamixel | /dev/ttyUSB3 | 26  | H54-200-S500-R | 2.0      | l_leg_an_r   | present_position, present_voltage, external_port_data_1, external_port_data_2, external_port_data_3, external_port_data_4
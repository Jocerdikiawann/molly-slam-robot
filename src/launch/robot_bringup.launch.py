import os
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        # ESP32 Bridge Node
        Node(
            package='robot_bringup',
            executable='esp32_bridge_node',
            name='esp32_bridge',
            output='screen',
            parameters=[{
                'port': '/dev/ttyUSB0', # USB ESP32 PORT /dev/ttyUSB0 biasanya
                'baudrate': 115200
            }]
        ),

        # RPLIDAR A1M8 Driver (install via: sudo apt install ros-<distro>-rplidar-ros)
        Node(
            package='rplidar_ros',
            executable='rplidar_node',
            name='rplidar_node',
            output='screen',
            parameters=[{
                'serial_port': '/dev/ttyUSB1', # port RPLIDAR
                'serial_baudrate': 115200,
                'frame_id': 'laser',
                'inverted': False,
                'angle_compensate': True
            }]
        ),

        # Pi Camera Module 3 (v4l2_camera)
        Node(
            package='v4l2_camera',
            executable='v4l2_camera_node',
            name='pi_cam_node',
            output='screen',
            parameters=[{
                'video_device': '/dev/video0',
                'image_size': [640, 480],
                'time_per_frame': [1, 30], # 30 FPS
                'camera_frame_id': 'camera_optical_frame'
            }]
        ),

        # Static Transforms (base_link -> sensors)
        # Laser TF (10cm maju dari pusat robot)
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            name='tf_base_to_laser',
            arguments=['0.1', '0.0', '0.12', '0.0', '0.0', '0.0', 'base_link', 'laser']
        ),
        # IMU TF (di pusat robot)
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            name='tf_base_to_imu',
            arguments=['0.0', '0.0', '0.05', '0.0', '0.0', '0.0', 'base_link', 'imu_link']
        ),
        # Camera TF (12cm maju, 15cm tinggi)
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            name='tf_base_to_cam',
            arguments=['0.12', '0.0', '0.15', '-1.57', '0.0', '-1.57', 'base_link', 'camera_optical_frame']
        ),
    ])

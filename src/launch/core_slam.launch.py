from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='rtabmap_slam',
            executable='rtabmap',
            name='rtabmap',
            output='screen',
            parameters=[{
                'frame_id': 'base_link',
                'odom_frame_id': 'odom',
                'map_frame_id': 'map',
                
                'subscribe_depth': False,   
                'subscribe_rgb': True,      
                'subscribe_scan': True,     
                'subscribe_odom': True,    
                
                'approx_sync': True,
                
                'Reg/Strategy': '1',               
                'RGBD/NeighborLinkRefining': 'true',
                'Mem/IncrementalMemory': 'true',
                'Mem/InitWMWithAllNodes': 'true'
            }],
            remappings=[
                ('rgb/image', '/camera/image_raw'),
                ('rgb/camera_info', '/camera/camera_info'),
                ('scan', '/scan'),
                ('odom', '/odom')
            ],
            arguments=['-d'] 
        ),

        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz2',
            output='screen'
        )
    ])

FROM docker.io/osrf/ros:humble-desktop-full

# Install driver RPLidar
RUN apt-get update && apt-get install -y \
  ros-humble-rplidar-ros \
  mesa-utils \
  libgl1-mesa-dri \
  && rm -rf /var/lib/apt/lists/*

RUN echo "source /opt/ros/humble/setup.bash" >> /root/.bashrc

WORKDIR /ros2_ws
CMD ["bash"]

sudo systemd-nspawn -b -D /var/lib/machines/ubuntu-jazzy \
  --bind=/tmp/.X11-unix \
  --bind=/dev/dri \
  --setenv=DISPLAY=$DISPLAY \
  --setenv=ROS_DOMAIN_ID=30 \
  --machine=ubuntu-jazzy

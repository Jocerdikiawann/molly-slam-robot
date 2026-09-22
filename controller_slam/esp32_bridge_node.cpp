#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <cmath>

class Esp32BridgeNode : public rclcpp::Node {
public:
  Esp32BridgeNode() : Node("esp32_bridge_node"), x_(0.0), y_(0.0), th_(0.0) {
    // Deklarasi Parameter
    this->declare_parameter<std::string>("port", "/dev/ttyUSB0");
    this->declare_parameter<int>("baudrate", 115200);

    std::string port = this->get_parameter("port").as_string();
    int baud = this->get_parameter("baudrate").as_int();

    // Inisialisasi Publisher
    imu_pub_ = this->create_publisher<sensor_msgs::msg::Imu>("/imu/data_raw", 20);
    odom_pub_ = this->create_publisher<nav_msgs::msg::Odometry>("/odom", 20);

    // Inisialisasi Subscriber cmd_vel
    cmd_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
      "/cmd_vel", 10, std::bind(&Esp32BridgeNode::cmdVelCallback, this, std::placeholders::_1));

    // Setup Serial Port
    if (!setupSerial(port, baud)) {
      RCLCPP_ERROR(this->get_logger(), "Gagal inisialisasi port: %s", port.c_str());
    }

    // Timer polling serial (100 Hz / 10 ms)
    last_time_ = this->now();
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(10),
      std::bind(&Esp32BridgeNode::readSerial, this)
    );
  }

  ~Esp32BridgeNode() {
    if (serial_fd_ >= 0) close(serial_fd_);
  }

private:
  bool setupSerial(const std::string &port, int baud) {
    serial_fd_ = open(port.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (serial_fd_ < 0) return false;

    struct termios tty;
    tcgetattr(serial_fd_, &tty);
    
    speed_t speed = (baud == 921600) ? B921600 : B115200;
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    tty.c_cflag |= (CLOCAL | CREAD | CS8);
    tty.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_oflag &= ~OPOST;

    tcsetattr(serial_fd_, TCSANOW, &tty);
    return true;
  }

  void cmdVelCallback(const geometry_msgs::msg::Twist::SharedPtr msg) {
    if (serial_fd_ < 0) return;
    // Format command ke ESP32: "CMD,<linear_x>,<angular_z>\n"
    std::string cmd = "CMD," + std::to_string(msg->linear.x) + "," + std::to_string(msg->angular.z) + "\n";
    write(serial_fd_, cmd.c_str(), cmd.length());
  }

  void readSerial() {
    if (serial_fd_ < 0) return;

    char buf[256];
    int n = read(serial_fd_, buf, sizeof(buf) - 1);
    if (n > 0) {
      buf[n] = '\0';
      serial_buffer_ += buf;

      size_t pos;
      while ((pos = serial_buffer_.find('\n')) != std::string::npos) {
        std::string line = serial_buffer_.substr(0, pos);
        serial_buffer_.erase(0, pos + 1);
        parseLine(line);
      }
    }
  }

  void parseLine(const std::string &line) {
    // Format expected from ESP32: 
    // DATA,ax,ay,az,gx,gy,gz,v_left,v_right
    std::stringstream ss(line);
    std::string tag;
    std::getline(ss, tag, ',');

    if (tag == "DATA") {
      float ax, ay, az, gx, gy, gz, v_l, v_r;
      std::string val;
      try {
        std::getline(ss, val, ','); ax = std::stof(val);
        std::getline(ss, val, ','); ay = std::stof(val);
        std::getline(ss, val, ','); az = std::stof(val);
        std::getline(ss, val, ','); gx = std::stof(val);
        std::getline(ss, val, ','); gy = std::stof(val);
        std::getline(ss, val, ','); gz = std::stof(val);
        std::getline(ss, val, ','); v_l = std::stof(val); // rad/s atau m/s
        std::getline(ss, val, ','); v_r = std::stof(val);
      } catch (...) { return; }

      auto current_time = this->now();
      double dt = (current_time - last_time_).seconds();
      last_time_ = current_time;

      // Publish IMU
      auto imu_msg = sensor_msgs::msg::Imu();
      imu_msg.header.stamp = current_time;
      imu_msg.header.frame_id = "imu_link";
      imu_msg.linear_acceleration.x = ax;
      imu_msg.linear_acceleration.y = ay;
      imu_msg.linear_acceleration.z = az;
      imu_msg.angular_velocity.x = gx;
      imu_msg.angular_velocity.y = gy;
      imu_msg.angular_velocity.z = gz;
      imu_pub_->publish(imu_msg);

      // Hitung & Publish Odometry (Differential Drive)
      // TODO: Update SOON (use omniwheel)
      double wheel_radius = 0.033; // 33 mm radius roda (omniwheel)
      double wheel_base = 0.16;   // 160 mm jarak antar roda
      double vx = ((v_r + v_l) / 2.0) * wheel_radius;
      double vth = ((v_r - v_l) / wheel_base) * wheel_radius;

      x_ += (vx * std::cos(th_)) * dt;
      y_ += (vx * std::sin(th_)) * dt;
      th_ += vth * dt;

      auto odom_msg = nav_msgs::msg::Odometry();
      odom_msg.header.stamp = current_time;
      odom_msg.header.frame_id = "odom";
      odom_msg.child_frame_id = "base_link";
      odom_msg.pose.pose.position.x = x_;
      odom_msg.pose.pose.position.y = y_;
      odom_msg.pose.pose.orientation.z = std::sin(th_ / 2.0);
      odom_msg.pose.pose.orientation.w = std::cos(th_ / 2.0);
      odom_msg.twist.twist.linear.x = vx;
      odom_msg.twist.twist.angular.z = vth;
      odom_pub_->publish(odom_msg);
    }
  }

  int serial_fd_ = -1;
  std::string serial_buffer_;
  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_pub_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_sub_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Time last_time_;
  double x_, y_, th_;
};

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Esp32BridgeNode>());
  rclcpp::shutdown();
  return 0;
}

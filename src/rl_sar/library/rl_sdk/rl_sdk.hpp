#ifndef RL_SDK_HPP
#define RL_SDK_HPP

#include <torch/script.h>
#include <iostream>
#include <string>

#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;

#include <yaml-cpp/yaml.h>
#define CONFIG_PATH CMAKE_CURRENT_SOURCE_DIR "/config.yaml"

template<typename T>
struct RobotCommand
{
    struct MotorCommand
    {
        std::vector<T> q = std::vector<T>(32, 0.0);
        std::vector<T> dq = std::vector<T>(32, 0.0);
        std::vector<T> tau = std::vector<T>(32, 0.0);
        std::vector<T> kp = std::vector<T>(32, 0.0);
        std::vector<T> kd = std::vector<T>(32, 0.0);
    } motor_command;
};

template<typename T>
struct RobotState
{
    struct IMU
    {
        T quaternion[4] = {1.0, 0.0, 0.0, 0.0}; // w, x, y, z
        T gyroscope[3] = {0.0, 0.0, 0.0};
        T accelerometer[3] = {0.0, 0.0, 0.0};
    } imu;

    struct MotorState
    {
        std::vector<T> q = std::vector<T>(32, 0.0);
        std::vector<T> dq = std::vector<T>(32, 0.0);
        std::vector<T> ddq = std::vector<T>(32, 0.0);
        std::vector<T> tauEst = std::vector<T>(32, 0.0);
        std::vector<T> cur = std::vector<T>(32, 0.0);
    } motor_state;
};

enum STATE {
    STATE_WAITING = 0,
    STATE_POS_GETUP,
    STATE_RL_INIT,
    STATE_RL_RUNNING,
    STATE_POS_GETDOWN,
};

struct KeyBoard
{
    STATE keyboard_state;
    double x = 0.0;
    double y = 0.0;
    double yaw = 0.0;
};

struct ModelParams
{
    std::string model_name;
    int num_observations;
    double damping;
    double stiffness;
    double action_scale;
    double hip_scale_reduction;
    std::vector<int> hip_scale_reduction_indices;
    int num_of_dofs;
    double lin_vel_scale;
    double ang_vel_scale;
    double dof_pos_scale;
    double dof_vel_scale;
    double clip_obs;
    double clip_actions;
    torch::Tensor torque_limits;
    torch::Tensor d_gains;
    torch::Tensor p_gains;
    torch::Tensor commands_scale;
    torch::Tensor default_dof_pos;
    std::vector<std::string> joint_names;
};

struct Observations
{
    torch::Tensor lin_vel;           
    torch::Tensor ang_vel;      
    torch::Tensor gravity_vec;      
    torch::Tensor commands;        
    torch::Tensor base_quat;   
    torch::Tensor dof_pos;           
    torch::Tensor dof_vel;           
    torch::Tensor actions;
};

class RL
{
public:
    RL(){};

    ModelParams params;
    Observations obs;

    virtual torch::Tensor Forward() = 0;
    virtual torch::Tensor ComputeObservation() = 0;
    torch::Tensor ComputeTorques(torch::Tensor actions);
    torch::Tensor ComputePosition(torch::Tensor actions);
    torch::Tensor QuatRotateInverse(torch::Tensor q, torch::Tensor v);
    void InitObservations();
    void InitOutputs();
    void InitKeyboard();
    void ReadYaml(std::string robot_name);
    std::string csv_filename;
    void CSVInit(std::string robot_name);
    void CSVLogger(torch::Tensor torque, torch::Tensor tau_est, torch::Tensor joint_pos, torch::Tensor joint_pos_target, torch::Tensor joint_vel);
    void run_keyboard();

    float getup_percent = 0.0;
    float getdown_percent = 0.0;
    std::vector<double> start_pos;
    std::vector<double> now_pos;

    int running_state = STATE_WAITING;

    RobotState<double> robot_state;
    RobotCommand<double> robot_command;

    virtual void GetState(RobotState<double> *state) = 0;
    virtual void SetCommand(const RobotCommand<double> *command) = 0;
    void StateController(const RobotState<double> *state, RobotCommand<double> *command);

protected:
    // rl module
    torch::jit::script::Module model;
    // observation buffer
    torch::Tensor lin_vel;           
    torch::Tensor ang_vel;      
    torch::Tensor gravity_vec;      
    torch::Tensor commands;        
    torch::Tensor base_quat;   
    torch::Tensor dof_pos;           
    torch::Tensor dof_vel;           
    torch::Tensor actions;
    // output buffer
    torch::Tensor output_torques;
    torch::Tensor output_dof_pos;
    // keyboard
    KeyBoard keyboard;
};

#endif // RL_SDK_HPP
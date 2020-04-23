/*******************************************************************************
* Copyright 2018 ROBOTIS CO., LTD.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/*
 *  manipulation_module.cpp
 *
 *  Created on: December 13, 2016
 *      Author: SCH
 */

#include "thormang3_manipulation_module/manipulation_module.h"

using namespace thormang3;

ManipulationModule::ManipulationModule()
  : control_cycle_sec_(0.008),
    is_init_moving_(false),
    is_left_moving_(false),
    is_right_moving_(false),
    is_left_arr_override_(false),
    is_right_arr_override_(false),
    flag_move_left(false),
    flag_move_right(false),
    ik_failed_left(false),
    ik_failed_right(false),
    ik_left_solving_(false),
    ik_right_solving_(false),
    arm_angle_display_(true)
{
  enable_       = false;
  module_name_  = "manipulation_module";
  control_mode_ = robotis_framework::PositionControl;

  /* arm */
  result_["r_arm_sh_p1"]  = new robotis_framework::DynamixelState();
  result_["l_arm_sh_p1"]  = new robotis_framework::DynamixelState();
  result_["r_arm_sh_r"]   = new robotis_framework::DynamixelState();
  result_["l_arm_sh_r"]   = new robotis_framework::DynamixelState();
  result_["r_arm_sh_p2"]  = new robotis_framework::DynamixelState();
  result_["l_arm_sh_p2"]  = new robotis_framework::DynamixelState();
  result_["r_arm_el_y"]   = new robotis_framework::DynamixelState();
  result_["l_arm_el_y"]   = new robotis_framework::DynamixelState();
  result_["r_arm_wr_r"]   = new robotis_framework::DynamixelState();
  result_["l_arm_wr_r"]   = new robotis_framework::DynamixelState();
  result_["r_arm_wr_y"]   = new robotis_framework::DynamixelState();
  result_["l_arm_wr_y"]   = new robotis_framework::DynamixelState();
  result_["r_arm_wr_p"]   = new robotis_framework::DynamixelState();
  result_["l_arm_wr_p"]   = new robotis_framework::DynamixelState();
  result_["torso_y"]      = new robotis_framework::DynamixelState();

  /* arm */
  joint_name_to_id_["r_arm_sh_p1"] = 1;
  joint_name_to_id_["l_arm_sh_p1"] = 2;
  joint_name_to_id_["r_arm_sh_r"]  = 3;
  joint_name_to_id_["l_arm_sh_r"]  = 4;
  joint_name_to_id_["r_arm_sh_p2"] = 5;
  joint_name_to_id_["l_arm_sh_p2"] = 6;
  joint_name_to_id_["r_arm_el_y"]  = 7;
  joint_name_to_id_["l_arm_el_y"]  = 8;
  joint_name_to_id_["r_arm_wr_r"]  = 9;
  joint_name_to_id_["l_arm_wr_r"]  = 10;
  joint_name_to_id_["r_arm_wr_y"]  = 11;
  joint_name_to_id_["l_arm_wr_y"]  = 12;
  joint_name_to_id_["r_arm_wr_p"]  = 13;
  joint_name_to_id_["l_arm_wr_p"]  = 14;
  joint_name_to_id_["torso_y"]     = 27;

  /* etc */
  joint_name_to_id_["r_arm_end"]   = 35;
  joint_name_to_id_["l_arm_end"]   = 34;

  /* parameter */
  present_joint_position_   = Eigen::VectorXd::Zero(MAX_JOINT_ID+1);
  goal_joint_position_      = Eigen::VectorXd::Zero(MAX_JOINT_ID+1);
  init_joint_position_      = Eigen::VectorXd::Zero(MAX_JOINT_ID+1);

  ik_id_left_start_  = 0;
  ik_id_left_end_    = 0;
  ik_id_right_start_  = 0;
  ik_id_right_end_    = 0;  

  ik_target_left_position_   = Eigen::MatrixXd::Zero(3,1);
  ik_target_right_position_  = Eigen::MatrixXd::Zero(3,1);
  ik_weight_                 = Eigen::MatrixXd::Zero(MAX_JOINT_ID+1, 1);
  ik_weight_.fill(1.0);

  robotis_              = new KinematicsDynamics(WholeBody);
}

ManipulationModule::~ManipulationModule()
{
  queue_thread_.join();
}

void ManipulationModule::initialize(const int control_cycle_msec, robotis_framework::Robot *robot)
{
  control_cycle_sec_ = control_cycle_msec * 0.001;
  // ROS_INFO_STREAM("control_cycle_sec: " << control_cycle_sec_);
  queue_thread_      = boost::thread(boost::bind(&ManipulationModule::queueThread, this));

  ros::NodeHandle ros_node;

  /* publish topics */

  // for gui
  status_msg_pub_     = ros_node.advertise<robotis_controller_msgs::StatusMsg>("/robotis/status", 1);
  movement_done_pub_  = ros_node.advertise<std_msgs::String>("/robotis/movement_done", 1);

  std::string _path = ros::package::getPath("thormang3_manipulation_module") + "/config/ik_weight.yaml";
  parseData(_path);
}

void ManipulationModule::parseData(const std::string &path)
{
  YAML::Node doc;
  try
  {
    // load yaml
    doc = YAML::LoadFile(path.c_str());
  } catch (const std::exception& e)
  {
    ROS_ERROR("Fail to load yaml file.");
    return;
  }

  YAML::Node ik_weight_node = doc["weight_value"];
  for (YAML::iterator it = ik_weight_node.begin(); it != ik_weight_node.end(); ++it)
  {
    int     id    = it->first.as<int>();
    double  value = it->second.as<double>();

    ik_weight_.coeffRef(id, 0) = value;
  }
}

void ManipulationModule::parseIniPoseData(const std::string &path)
{
  YAML::Node doc;
  try
  {
    // load yaml
    doc = YAML::LoadFile(path.c_str());
  } catch (const std::exception& e)
  {
    ROS_ERROR("Fail to load yaml file.");
    return;
  }

  // parse movement time
  double mov_time = doc["mov_time"].as<double>();
  mov_init_time_ = mov_time;

  // parse target pose
  YAML::Node tar_pose_node = doc["tar_pose"];
  for (YAML::iterator it = tar_pose_node.begin(); it != tar_pose_node.end(); ++it)
  {
    int     id    = it->first.as<int>();
    double  value = it->second.as<double>();

    init_joint_position_(id) = value * DEGREE2RADIAN;
  }

  all_init_time_steps_ = int(mov_init_time_ / control_cycle_sec_) + 1;
  goal_joint_tra_.resize(all_init_time_steps_, MAX_JOINT_ID + 1);
}

void ManipulationModule::queueThread()
{
  ros::NodeHandle     ros_node;
  ros::CallbackQueue  callback_queue;

  ros_node.setCallbackQueue(&callback_queue);

  /* subscribe topics */
  ros::Subscriber ini_pose_msg_sub              = ros_node.subscribe("/robotis/manipulation/ini_pose_msg", 5,
                                                        &ManipulationModule::initPoseMsgCallback, this);
  ros::Subscriber kinematics_pose_msg_sub       = ros_node.subscribe("/robotis/manipulation/kinematics_pose_msg", 5,
                                                               &ManipulationModule::kinematicsPoseMsgCallback, this);
  ros::Subscriber kinematics_pose_arr_msg_sub   = ros_node.subscribe("/robotis/manipulation/kinematics_pose_arr_msg", 5,
                                                               &ManipulationModule::kinematicsPoseArrMsgCallback, this);
  /* service */
  ros::ServiceServer get_kinematics_pose_server = ros_node.advertiseService("/robotis/manipulation/get_kinematics_pose",
                                                                            &ManipulationModule::getKinematicsPoseCallback, this);

  ros::WallDuration duration(control_cycle_sec_);
  while(ros_node.ok())
    callback_queue.callAvailable(duration);
}

void ManipulationModule::initPoseMsgCallback(const std_msgs::String::ConstPtr& msg)
{
  if (enable_ == false)
    return;

  if (is_left_moving_ == false && is_right_moving_ == false && is_init_moving_ == false 
        && is_left_arr_override_ == false && is_right_arr_override_ == false)
  {
    if (msg->data == "ini_pose")
    {
      // parse initial pose
      std::string ini_pose_path = ros::package::getPath("thormang3_manipulation_module") + "/config/ini_pose.yaml";
      parseIniPoseData(ini_pose_path);

      traj_generate_thread_ = new boost::thread(boost::bind(&ManipulationModule::initPoseTrajGenerateProc, this));
      delete traj_generate_thread_;
    }
    else if (msg->data == "align_keyboard_pose")
    {
      // parse initial pose
      std::string ini_pose_path = ros::package::getPath("thormang3_manipulation_module") + "/config/align_keyboard_pose.yaml";
      parseIniPoseData(ini_pose_path);

      traj_generate_thread_ = new boost::thread(boost::bind(&ManipulationModule::initPoseTrajGenerateProc, this));
      delete traj_generate_thread_;
    }
    else if (msg->data == "typing_pose")
    {
      // parse initial pose
      std::string ini_pose_path = ros::package::getPath("thormang3_manipulation_module") + "/config/typing_pose.yaml";
      parseIniPoseData(ini_pose_path);

      traj_generate_thread_ = new boost::thread(boost::bind(&ManipulationModule::initPoseTrajGenerateProc, this));
      delete traj_generate_thread_;
    }
  }
  else
  {
    ROS_INFO("previous task is alive");
  }

  movement_done_msg_.data = "manipulation_init";
  return;
}

bool ManipulationModule::getKinematicsPoseCallback(thormang3_manipulation_module_msgs::GetKinematicsPose::Request &req,
                                                   thormang3_manipulation_module_msgs::GetKinematicsPose::Response &res)
{
  if (enable_ == false)
    return false;

  int end_index;

  if (req.group_name == "left_arm")
    end_index = ID_L_ARM_END;
  else if (req.group_name == "right_arm")
    end_index = ID_R_ARM_END;
  else if (req.group_name == "left_arm_with_torso")
    end_index = ID_L_ARM_END;
  else if (req.group_name == "right_arm_with_torso")
    end_index = ID_R_ARM_END;
  else
    return false;

  res.group_pose.position.x = robotis_->thormang3_link_data_[end_index]->position_.coeff(0, 0);
  res.group_pose.position.y = robotis_->thormang3_link_data_[end_index]->position_.coeff(1, 0);
  res.group_pose.position.z = robotis_->thormang3_link_data_[end_index]->position_.coeff(2, 0);

  Eigen::Quaterniond quaternion = robotis_framework::convertRotationToQuaternion(robotis_->thormang3_link_data_[end_index]->orientation_);

  res.group_pose.orientation.w = quaternion.w();
  res.group_pose.orientation.x = quaternion.x();
  res.group_pose.orientation.y = quaternion.y();
  res.group_pose.orientation.z = quaternion.z();

  return true;
}

void ManipulationModule::kinematicsPoseMsgCallback(const thormang3_manipulation_module_msgs::KinematicsPose::ConstPtr& msg)
{
  if (enable_ == false)
    return;

  movement_done_msg_.data = "manipulation";

  if (msg->name == "left_arm" || msg->name == "left_arm_with_torso")
  {
    goal_kinematics_left_pose_msg_ = *msg;

    if (msg->name == "left_arm")
    {
      ik_id_left_start_  = ID_L_ARM_START;
      ik_id_left_end_    = ID_L_ARM_END;
    }
    else if (msg->name == "left_arm_with_torso")
    {
      ik_id_left_start_  = ID_TORSO;
      ik_id_left_end_    = ID_L_ARM_END;
    }

    if (is_left_moving_ == false && is_init_moving_ == false && is_left_arr_override_ == false)
    {
      traj_left_generate_thread_ = new boost::thread(boost::bind(&ManipulationModule::taskLeftTrajGenerateProc, this));
      delete traj_left_generate_thread_;
    }
    else
    {
      ROS_INFO("previous left task is alive");
    }
  }

  else if (msg->name == "right_arm" || msg->name == "right_arm_with_torso")
  {
    goal_kinematics_right_pose_msg_ = *msg;

    if (msg->name == "right_arm")
    {
      ik_id_right_start_  = ID_R_ARM_START;
      ik_id_right_end_    = ID_R_ARM_END;
    }

    else if (msg->name == "right_arm_with_torso")
    {
      ik_id_right_start_  = ID_TORSO;
      ik_id_right_end_    = ID_R_ARM_END;
    }

    if (is_right_moving_ == false && is_init_moving_ == false && is_right_arr_override_ == false)
    {
      traj_right_generate_thread_ = new boost::thread(boost::bind(&ManipulationModule::taskRightTrajGenerateProc, this));
      delete traj_right_generate_thread_;
    }
    else
    {
      ROS_INFO("previous right task is alive");
    }
  }
  return;
}

void ManipulationModule::kinematicsPoseArrMsgCallback(const thormang3_manipulation_module_msgs::KinematicsArrayPose::ConstPtr& msg)
{
  // ROS_INFO_STREAM(*msg);
  if (enable_ == false)
    return;

  movement_done_msg_.data = "manipulation";

  if (msg->name == "left_arm" || msg->name == "left_arm_with_torso")
  {
    if (msg->name == "left_arm")
    {
      ik_id_left_start_  = ID_L_ARM_START;
      ik_id_left_end_    = ID_L_ARM_END;
    }
    else if (msg->name == "left_arm_with_torso")
    {
      ik_id_left_start_  = ID_TORSO;
      ik_id_left_end_    = ID_L_ARM_END;
    }

    if (is_left_moving_ == false && is_init_moving_ == false && is_left_arr_override_ == false)
    {
      goal_kinematics_left_arr_pose_msg_ = *msg;
      is_left_arr_override_ = true;
      traj_left_generate_thread_ = new boost::thread(boost::bind(&ManipulationModule::taskLeftArrTrajGenerateProc, this));
      delete traj_left_generate_thread_;
    }
    else
    {
      ROS_INFO("previous left arr task is alive");
    }
  }
  else if (msg->name == "right_arm" || msg->name == "right_arm_with_torso")
  {
    if (msg->name == "right_arm")
    {
      ik_id_right_start_  = ID_R_ARM_START;
      ik_id_right_end_    = ID_R_ARM_END;
    }

    else if (msg->name == "right_arm_with_torso")
    {
      ik_id_right_start_  = ID_TORSO;
      ik_id_right_end_    = ID_R_ARM_END;
    }

    if (is_right_moving_ == false && is_init_moving_ == false && is_right_arr_override_ == false)
    {
      goal_kinematics_right_arr_pose_msg_ = *msg;
      is_right_arr_override_ = true;
      traj_right_generate_thread_ = new boost::thread(boost::bind(&ManipulationModule::taskRightArrTrajGenerateProc, this));
      delete traj_right_generate_thread_;
    }
    else
    {
      ROS_INFO("previous right arr task is alive");
    }
  }
  return;
}

void ManipulationModule::initPoseTrajGenerateProc()
{
  for (int id = 1; id <= MAX_JOINT_ID; id++)
  {
    double ini_value = goal_joint_position_(id);
    double tar_value = init_joint_position_(id);

    Eigen::MatrixXd tra = robotis_framework::calcMinimumJerkTra(ini_value, 0.0, 0.0,
                                                                tar_value, 0.0, 0.0,
                                                                control_cycle_sec_, mov_init_time_);
    goal_joint_tra_.block(0, id, all_init_time_steps_, 1) = tra;
  }
  cnt_init_        = 0;
  is_init_moving_  = true;
  ik_failed_left   = false;
  ik_failed_right  = false;
  ROS_INFO("[start] manipulation init send trajectory");
}

void ManipulationModule::taskLeftTrajGenerateProc()
{
  if (is_left_arr_override_ == true)
  {
    mov_left_time_ = goal_kinematics_left_arr_pose_msg_.time;
  }
  else
  {    
    if (goal_kinematics_left_pose_msg_.time <= 0.0)
    {
      /* set movement time */
      double tol      = 0.1; // m per sec
      double mov_time = 2.0;

      double diff     = sqrt(
                            pow(robotis_->thormang3_link_data_[ik_id_left_end_]->position_.coeff(0,0) - goal_kinematics_left_pose_msg_.pose.position.x, 2)
                          + pow(robotis_->thormang3_link_data_[ik_id_left_end_]->position_.coeff(1,0) - goal_kinematics_left_pose_msg_.pose.position.y, 2)
                          + pow(robotis_->thormang3_link_data_[ik_id_left_end_]->position_.coeff(2,0) - goal_kinematics_left_pose_msg_.pose.position.z, 2)
                        );

      mov_left_time_ = diff / tol;
      int all_time_steps = int(floor((mov_left_time_ / control_cycle_sec_) + 1.0));
      mov_left_time_ = double(all_time_steps - 1) * control_cycle_sec_;

      if (mov_left_time_ < mov_time)
        mov_left_time_ = mov_time;
    }
    else
    {
      mov_left_time_ = goal_kinematics_left_pose_msg_.time;
    }
  }

  all_left_time_steps_ = int(mov_left_time_ / control_cycle_sec_) + 1;
  goal_left_task_tra_.resize(all_left_time_steps_, 3);

  /* calculate trajectory */
  for (int dim = 0; dim < 3; dim++)
  {
    double ini_value = robotis_->thormang3_link_data_[ik_id_left_end_]->position_.coeff(dim, 0);
    double tar_value;
    if (is_left_arr_override_ == true)
    {
      if (dim == 0)
        tar_value = left_sub_pose_msg_.position.x;
      else if (dim == 1)
        tar_value = left_sub_pose_msg_.position.y;
      else if (dim == 2)
        tar_value = left_sub_pose_msg_.position.z;  
    }
    else
    {
      if (dim == 0)
        tar_value = goal_kinematics_left_pose_msg_.pose.position.x;
      else if (dim == 1)
        tar_value = goal_kinematics_left_pose_msg_.pose.position.y;
      else if (dim == 2)
        tar_value = goal_kinematics_left_pose_msg_.pose.position.z;  
    }    

    Eigen::MatrixXd tra = robotis_framework::calcMinimumJerkTra(ini_value, 0.0, 0.0, tar_value, 0.0, 0.0,
                                                                control_cycle_sec_,
                                                                mov_left_time_);

    goal_left_task_tra_.block(0, dim, all_left_time_steps_, 1) = tra;
  }

  cnt_left_          = 0;
  is_left_moving_    = true;
  ik_left_solving_   = true;

  if (!is_left_arr_override_)
    ROS_INFO("[start] taskLeftTrajGenerateProc send trajectory");
}

void ManipulationModule::taskRightTrajGenerateProc()
{
  if (is_right_arr_override_ == true)
  {
    mov_right_time_ = goal_kinematics_right_arr_pose_msg_.time;
  }
  else
  {
    if (goal_kinematics_right_pose_msg_.time <= 0.0)
    {
      /* set movement time */
      double tol      = 0.1; // m per sec
      double mov_time = 2.0;

      double diff     = sqrt(
                            pow(robotis_->thormang3_link_data_[ik_id_right_end_]->position_.coeff(0,0) - goal_kinematics_right_pose_msg_.pose.position.x, 2)
                          + pow(robotis_->thormang3_link_data_[ik_id_right_end_]->position_.coeff(1,0) - goal_kinematics_right_pose_msg_.pose.position.y, 2)
                          + pow(robotis_->thormang3_link_data_[ik_id_right_end_]->position_.coeff(2,0) - goal_kinematics_right_pose_msg_.pose.position.z, 2)
                        );

      mov_right_time_ = diff / tol;
      int all_time_steps = int(floor((mov_right_time_ / control_cycle_sec_) + 1.0));
      mov_right_time_ = double(all_time_steps - 1) * control_cycle_sec_;

      if (mov_right_time_ < mov_time)
        mov_right_time_ = mov_time;
    }
    else
    {
      mov_right_time_ = goal_kinematics_right_pose_msg_.time;
    }
  }
  
  all_right_time_steps_ = int(mov_right_time_ / control_cycle_sec_) + 1;
  goal_right_task_tra_.resize(all_right_time_steps_, 3);

  /* calculate trajectory */
  for (int dim = 0; dim < 3; dim++)
  {
    double ini_value = robotis_->thormang3_link_data_[ik_id_right_end_]->position_.coeff(dim, 0);
    double tar_value;
    if (is_right_arr_override_ == true)
    {
      if (dim == 0)
        tar_value = right_sub_pose_msg_.position.x;
      else if (dim == 1)
        tar_value = right_sub_pose_msg_.position.y;
      else if (dim == 2)
        tar_value = right_sub_pose_msg_.position.z;  
    }
    else
    {
      if (dim == 0)
        tar_value = goal_kinematics_right_pose_msg_.pose.position.x;
      else if (dim == 1)
        tar_value = goal_kinematics_right_pose_msg_.pose.position.y;
      else if (dim == 2)
        tar_value = goal_kinematics_right_pose_msg_.pose.position.z;
    }

    Eigen::MatrixXd tra = robotis_framework::calcMinimumJerkTra(ini_value, 0.0, 0.0, tar_value, 0.0, 0.0,
                                                                control_cycle_sec_,
                                                                mov_right_time_);

    goal_right_task_tra_.block(0, dim, all_right_time_steps_, 1) = tra;
  }

  cnt_right_         = 0;
  is_right_moving_   = true;
  ik_right_solving_  = true;

  if (!is_right_arr_override_)
    ROS_INFO("[start] taskRightTrajGenerateProc send trajectory");
}

void ManipulationModule::taskLeftArrTrajGenerateProc()
{  
  if (is_left_arr_override_ == true)
  {
    ROS_INFO("[start] send left arr trajectory");
    publishStatusMsg(robotis_controller_msgs::StatusMsg::STATUS_INFO, "Start Left Arm Arr Trajectory");
    for(auto const& sub_pose: goal_kinematics_left_arr_pose_msg_.pose.poses)
    {
      left_sub_pose_msg_ = sub_pose;

      while(flag_move_left == true); // wait until to be false

      if (ik_failed_left== true)
      {
        ROS_INFO("[end] ik failed calculate left arr trajectory");
        break;
      }

      if (is_left_moving_ == false && is_init_moving_ == false)
      {
        flag_move_left = true;
        traj_left_generate_thread_ = new boost::thread(boost::bind(&ManipulationModule::taskLeftTrajGenerateProc, this));
        delete traj_left_generate_thread_;
      }
    }

    while(flag_move_left == true); // wait until to be false

    is_left_arr_override_ = false;
    ROS_INFO("[end] finish send left arr trajectory");
    publishStatusMsg(robotis_controller_msgs::StatusMsg::STATUS_INFO, "Finish Left Arm Arr Trajectory");
  }
}

void ManipulationModule::taskRightArrTrajGenerateProc()
{  
  if (is_right_arr_override_ == true)
  {
    ROS_INFO("[start] send right arr trajectory");
    publishStatusMsg(robotis_controller_msgs::StatusMsg::STATUS_INFO, "Start Right Arm Arr Trajectory");
    for(auto const& sub_pose: goal_kinematics_right_arr_pose_msg_.pose.poses)
    {
      right_sub_pose_msg_ = sub_pose;

      while(flag_move_right == true); // wait until to be false

      if (ik_failed_right== true)
      {
        ROS_INFO("[end] ik failed calculate right arr trajectory");
        break;
      }

      if (is_right_moving_ == false && is_init_moving_ == false)
      {
        flag_move_right = true;
        traj_right_generate_thread_ = new boost::thread(boost::bind(&ManipulationModule::taskRightTrajGenerateProc, this));
        delete traj_right_generate_thread_;
      }
    }

    while(flag_move_right == true); // wait until to be false

    is_right_arr_override_ = false; 
    ROS_INFO("[end] finish send right arr trajectory");
    publishStatusMsg(robotis_controller_msgs::StatusMsg::STATUS_INFO, "Finish Right Arm Arr Trajectory");
  }
}

void ManipulationModule::setInverseKinematics(int cnt, Eigen::MatrixXd start_rotation, int all_time_steps, Eigen::MatrixXd goal_task_tra_,
                                              Eigen::MatrixXd &ik_target_position_, Eigen::MatrixXd &ik_target_rotation_,
                                              thormang3_manipulation_module_msgs::KinematicsPose  goal_kinematics_pose_msg_)
{
  for (int dim = 0; dim < 3; dim++)
    ik_target_position_.coeffRef(dim, 0) = goal_task_tra_.coeff(cnt, dim);

  Eigen::Quaterniond start_quaternion = robotis_framework::convertRotationToQuaternion(start_rotation);
  Eigen::Quaterniond target_quaternion(goal_kinematics_pose_msg_.pose.orientation.w,
                                       goal_kinematics_pose_msg_.pose.orientation.x,
                                       goal_kinematics_pose_msg_.pose.orientation.y,
                                       goal_kinematics_pose_msg_.pose.orientation.z);
  double count = (double) cnt / (double) all_time_steps;
  Eigen::Quaterniond _quaternion = start_quaternion.slerp(count, target_quaternion);

  ik_target_rotation_ = robotis_framework::convertQuaternionToRotation(_quaternion);
}

void ManipulationModule::setInverseKinematicsArr(int cnt, Eigen::MatrixXd start_rotation, int all_time_steps, Eigen::MatrixXd goal_task_tra_,
                                              Eigen::MatrixXd &ik_target_position_, Eigen::MatrixXd &ik_target_rotation_,
                                              geometry_msgs::Pose sub_pose_msg_)
{
  for (int dim = 0; dim < 3; dim++)
    ik_target_position_.coeffRef(dim, 0) = goal_task_tra_.coeff(cnt, dim);

  Eigen::Quaterniond start_quaternion = robotis_framework::convertRotationToQuaternion(start_rotation);
  Eigen::Quaterniond target_quaternion(sub_pose_msg_.orientation.w,
                                       sub_pose_msg_.orientation.x,
                                       sub_pose_msg_.orientation.y,
                                       sub_pose_msg_.orientation.z);
  double count = (double) cnt / (double) all_time_steps;
  Eigen::Quaterniond _quaternion = start_quaternion.slerp(count, target_quaternion);

  ik_target_rotation_ = robotis_framework::convertQuaternionToRotation(_quaternion);
}

void ManipulationModule::process(std::map<std::string, robotis_framework::Dynamixel *> dxls,
                                 std::map<std::string, double> sensors)
{
  if (enable_ == false)
    return;

  /*----- write curr position -----*/
  for (std::map<std::string, robotis_framework::DynamixelState *>::iterator state_iter = result_.begin();
       state_iter != result_.end(); state_iter++)
  {
    std::string joint_name = state_iter->first;

    robotis_framework::Dynamixel *dxl = NULL;
    std::map<std::string, robotis_framework::Dynamixel*>::iterator dxl_it = dxls.find(joint_name);
    if (dxl_it != dxls.end())
      dxl = dxl_it->second;
    else
      continue;

    double joint_curr_position = dxl->dxl_state_->present_position_;
    double joint_goal_position = dxl->dxl_state_->goal_position_;

    // ROS_INFO_STREAM(joint_name << joint_goal_position);

    present_joint_position_(joint_name_to_id_[joint_name]) = joint_curr_position;
    goal_joint_position_(joint_name_to_id_[joint_name]) = joint_goal_position;
  }

  /*----- forward kinematics -----*/
  for (int id = 1; id <= MAX_JOINT_ID; id++)
    robotis_->thormang3_link_data_[id]->joint_angle_ = goal_joint_position_(id);

  robotis_->calcForwardKinematics(0);

  /* ----- send trajectory ----- */

  if (is_left_moving_ == true)
  {
    if (cnt_left_ == 0)
    {
      if (is_left_arr_override_ == false)
        publishStatusMsg(robotis_controller_msgs::StatusMsg::STATUS_INFO, "Start Left Arm Trajectory");
      ik_start_left_rotation_ = robotis_->thormang3_link_data_[ik_id_left_end_]->orientation_;
    }

    if (ik_left_solving_ == true)
    {
      /* ----- inverse kinematics ----- */
      if (is_left_arr_override_)
      {
        setInverseKinematicsArr(cnt_left_, ik_start_left_rotation_, all_left_time_steps_, goal_left_task_tra_,
                              ik_target_left_position_, ik_target_left_rotation_, left_sub_pose_msg_);
      }
      else
      {
        setInverseKinematics(cnt_left_, ik_start_left_rotation_, all_left_time_steps_, goal_left_task_tra_,
                              ik_target_left_position_, ik_target_left_rotation_, goal_kinematics_left_pose_msg_);
      }

      int     max_iter         = 30;
      double  ik_tol           = 1e-3;
      bool    ik_left_success  = robotis_->calcInverseKinematics(ik_id_left_start_, ik_id_left_end_,
                                                            ik_target_left_position_, ik_target_left_rotation_,
                                                            max_iter, ik_tol, ik_weight_);

      if (ik_left_success == true)
      {
        for (int id = 1; id <= MAX_JOINT_ID; id++)
        {
          if (id % 2 == 0) // genap (even)
            goal_joint_position_(id) = robotis_->thormang3_link_data_[id]->joint_angle_;
        }
      }
      else
      {
        ROS_ERROR("----- ik left arm failed -----");
        ROS_ERROR("[end] send trajectory");

        publishStatusMsg(robotis_controller_msgs::StatusMsg::STATUS_ERROR, "IK Failed");

        if (is_left_arr_override_ == true)
        {
          ik_failed_left   = true;
          flag_move_left   = false;
          is_left_arr_override_ = false;
        }
        
        is_left_moving_  = false;
        ik_left_solving_ = false;
        cnt_left_        = 0;

        movement_done_msg_.data = "manipulation_fail";
        movement_done_pub_.publish(movement_done_msg_);
        movement_done_msg_.data = "";
      }
    }
    else
    {
      for (int id = 1; id <= MAX_JOINT_ID; id++)
        if (id % 2 == 0)
          goal_joint_position_(id) = goal_joint_tra_(cnt_left_, id);
    }
    cnt_left_++;
  }

  if (is_right_moving_ == true)
  {
    if (cnt_right_ == 0)
    {
      if (is_right_arr_override_ == false)
        publishStatusMsg(robotis_controller_msgs::StatusMsg::STATUS_INFO, "Start Right Arm Trajectory");
      ik_start_right_rotation_ = robotis_->thormang3_link_data_[ik_id_right_end_]->orientation_;
    }

    if (ik_right_solving_ == true)
    {
      /* ----- inverse kinematics ----- */
      if (is_right_arr_override_)
      {
        setInverseKinematicsArr(cnt_right_, ik_start_right_rotation_, all_right_time_steps_, goal_right_task_tra_,
                              ik_target_right_position_, ik_target_right_rotation_, right_sub_pose_msg_);
      }
      else
      {
        setInverseKinematics(cnt_right_, ik_start_right_rotation_, all_right_time_steps_, goal_right_task_tra_,
                            ik_target_right_position_, ik_target_right_rotation_, goal_kinematics_right_pose_msg_);
      }
      
      int     max_iter         = 30;
      double  ik_tol           = 1e-3;
      bool    ik_right_success  = robotis_->calcInverseKinematics(ik_id_right_start_, ik_id_right_end_,
                                                            ik_target_right_position_, ik_target_right_rotation_,
                                                            max_iter, ik_tol, ik_weight_);

      if (ik_right_success == true)
      {
        for (int id = 1; id <= MAX_JOINT_ID; id++)
        {
          if (id % 2 != 0) // ganjil (odd)
            goal_joint_position_(id) = robotis_->thormang3_link_data_[id]->joint_angle_;
        }
      }
      else
      {
        ROS_ERROR("----- ik right arm failed -----");
        ROS_ERROR("[end] send trajectory");

        publishStatusMsg(robotis_controller_msgs::StatusMsg::STATUS_ERROR, "IK Failed");

        if (is_right_arr_override_ == true)
        {
          ik_failed_right   = true;
          flag_move_right   = false;
          is_right_arr_override_ = false;
        }

        is_right_moving_  = false;
        ik_right_solving_ = false;
        cnt_right_        = 0;

        movement_done_msg_.data = "manipulation_fail";
        movement_done_pub_.publish(movement_done_msg_);
        movement_done_msg_.data = "";
      }
    }
    else
    {
      for (int id = 1; id <= MAX_JOINT_ID; id++)
        if (id % 2 != 0) // && id != 13)  // ganjil (odd)
          goal_joint_position_(id) = goal_joint_tra_(cnt_right_, id);
    }
    cnt_right_++;
  }

  if (is_init_moving_ == true)
  {
    if (cnt_init_ == 0)
      publishStatusMsg(robotis_controller_msgs::StatusMsg::STATUS_INFO, "Start Init Trajectory");

    for (int id = 1; id <= MAX_JOINT_ID; id++)
      goal_joint_position_(id) = goal_joint_tra_(cnt_init_, id);

    cnt_init_++;
  }

  /*----- set joint data -----*/
  for (std::map<std::string, robotis_framework::DynamixelState *>::iterator state_iter = result_.begin();
      state_iter != result_.end(); state_iter++)
  {
    std::string joint_name = state_iter->first;

    // if ( joint_name.compare("l_arm_wr_r") == 0 )
    // {
    //   result_[joint_name]->goal_position_ = 3.14;
    //   // ROS_INFO_STREAM(joint_name);
    // }
    // else if ( joint_name.compare("l_arm_wr_p") == 0 )
    // {
    //   result_[joint_name]->goal_position_ = -0.3;
    //   // ROS_INFO_STREAM(joint_name);
    // }
    // else
    //   result_[joint_name]->goal_position_ = goal_joint_position_(joint_name_to_id_[joint_name]);
    result_[joint_name]->goal_position_ = goal_joint_position_(joint_name_to_id_[joint_name]);
  }

  /*---------- initialize count number ----------*/
  if (is_left_moving_ == true)
  {
    if (cnt_left_ >= all_left_time_steps_)
    {
      if (!is_left_arr_override_)
      {
        ROS_INFO("[end] success send left trajectory");
        publishStatusMsg(robotis_controller_msgs::StatusMsg::STATUS_INFO, "End Left Arm Trajectory");
      }
        
      is_left_moving_  = false;
      ik_left_solving_ = false;
      cnt_left_        = 0;

      if (is_left_arr_override_ == true)
        flag_move_left = false;

      movement_done_pub_.publish(movement_done_msg_);
      movement_done_msg_.data = "";

      if (arm_angle_display_ == true)
      {
        ROS_INFO("l_arm_sh_p1 : %f", goal_joint_position_(joint_name_to_id_["l_arm_sh_p1"]) * RADIAN2DEGREE );
        ROS_INFO("l_arm_sh_r  : %f", goal_joint_position_(joint_name_to_id_["l_arm_sh_r"])  * RADIAN2DEGREE );
        ROS_INFO("l_arm_sh_p2 : %f", goal_joint_position_(joint_name_to_id_["l_arm_sh_p2"]) * RADIAN2DEGREE );
        ROS_INFO("l_arm_el_y  : %f", goal_joint_position_(joint_name_to_id_["l_arm_el_y"])  * RADIAN2DEGREE );
        ROS_INFO("l_arm_wr_r  : %f", goal_joint_position_(joint_name_to_id_["l_arm_wr_r"])  * RADIAN2DEGREE );
        ROS_INFO("l_arm_wr_y  : %f", goal_joint_position_(joint_name_to_id_["l_arm_wr_y"])  * RADIAN2DEGREE );
        ROS_INFO("l_arm_wr_p  : %f", goal_joint_position_(joint_name_to_id_["l_arm_wr_p"])  * RADIAN2DEGREE );
        ROS_INFO("rad l_arm_wr_r  : %f", goal_joint_position_(joint_name_to_id_["l_arm_wr_r"]) );
        ROS_INFO("rad l_arm_wr_y  : %f", goal_joint_position_(joint_name_to_id_["l_arm_wr_y"]) );
        ROS_INFO("rad l_arm_wr_p  : %f", goal_joint_position_(joint_name_to_id_["l_arm_wr_p"]) );
      }
    }
  }

  if (is_right_moving_ == true)
  {
    if (cnt_right_ >= all_right_time_steps_)
    {
      if (!is_right_arr_override_)
      {
        ROS_INFO("[end] success send right trajectory");
        publishStatusMsg(robotis_controller_msgs::StatusMsg::STATUS_INFO, "End Right Arm Trajectory");
      }        

      is_right_moving_  = false;
      ik_right_solving_ = false;
      cnt_right_        = 0;

      if (is_right_arr_override_ == true)
        flag_move_right = false;

      movement_done_pub_.publish(movement_done_msg_);
      movement_done_msg_.data = "";

      if (arm_angle_display_ == true)
      {
        ROS_INFO("r_arm_sh_p1 : %f", goal_joint_position_(joint_name_to_id_["r_arm_sh_p1"]) * RADIAN2DEGREE );
        ROS_INFO("r_arm_sh_r  : %f", goal_joint_position_(joint_name_to_id_["r_arm_sh_r"])  * RADIAN2DEGREE );
        ROS_INFO("r_arm_sh_p2 : %f", goal_joint_position_(joint_name_to_id_["r_arm_sh_p2"]) * RADIAN2DEGREE );
        ROS_INFO("r_arm_el_y  : %f", goal_joint_position_(joint_name_to_id_["r_arm_el_y"])  * RADIAN2DEGREE );
        ROS_INFO("r_arm_wr_r  : %f", goal_joint_position_(joint_name_to_id_["r_arm_wr_r"])  * RADIAN2DEGREE );
        ROS_INFO("r_arm_wr_y  : %f", goal_joint_position_(joint_name_to_id_["r_arm_wr_y"])  * RADIAN2DEGREE );
        ROS_INFO("r_arm_wr_p  : %f", goal_joint_position_(joint_name_to_id_["r_arm_wr_p"])  * RADIAN2DEGREE );
        ROS_INFO("rad r_arm_wr_r  : %f", goal_joint_position_(joint_name_to_id_["r_arm_wr_r"]) );
        ROS_INFO("rad r_arm_wr_y  : %f", goal_joint_position_(joint_name_to_id_["r_arm_wr_y"]) );
        ROS_INFO("rad r_arm_wr_p  : %f", goal_joint_position_(joint_name_to_id_["r_arm_wr_p"]) );
      }
    }
  }

  if (is_init_moving_ == true)
  {
    if (cnt_init_ >= all_init_time_steps_)
    {
      ROS_INFO("[end] success manipulation send init trajectory");
      publishStatusMsg(robotis_controller_msgs::StatusMsg::STATUS_INFO, "End Init Trajectory");

      is_init_moving_  = false;
      cnt_init_        = 0;
      movement_done_pub_.publish(movement_done_msg_);
      movement_done_msg_.data = "";
    }
  }
}

void ManipulationModule::stop()
{
  is_init_moving_         = false;
  is_left_moving_         = false;
  is_right_moving_        = false;
  ik_left_solving_        = false;
  ik_right_solving_       = false;
  is_left_arr_override_   = false;
  is_right_arr_override_  = false;
  flag_move_left          = false;
  flag_move_right         = false;
  cnt_init_               = 0;
  cnt_left_               = 0;
  return;
}

bool ManipulationModule::isRunning()
{
  return is_left_moving_ || is_right_moving_ || is_init_moving_;
}

void ManipulationModule::publishStatusMsg(unsigned int type, std::string msg)
{
  robotis_controller_msgs::StatusMsg status;
  status.header.stamp = ros::Time::now();
  status.type         = type;
  status.module_name  = "Manipulation";
  status.status_msg   = msg;

  status_msg_pub_.publish(status);
}

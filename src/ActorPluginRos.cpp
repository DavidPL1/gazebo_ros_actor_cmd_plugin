/*
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
 *
*/
/**
 *  \author David Leins
 *  \date 28th of November 2018
 *  \desc Gazebo ros plugin to steer an actor with twist messages over ros. Based on actor_plugin. 
 */

#include "actor_plugin_ros/ActorPluginRos.hpp"

using namespace gazebo;
GZ_REGISTER_MODEL_PLUGIN(ActorPluginRos)

#define WALKING_ANIMATION "walking"

/////////////////////////////////////////////////
ActorPluginRos::ActorPluginRos()
{
}

/////////////////////////////////////////////////
void ActorPluginRos::Load(physics::ModelPtr _model, sdf::ElementPtr _sdf)
{

  // Make sure the ROS node for Gazebo has already been initialized                                                                                    
  if (!ros::isInitialized())
  {
    ROS_FATAL_STREAM("A ROS node for Gazebo has not been initialized, unable to load plugin. "
      << "Load the Gazebo system plugin 'libgazebo_ros_api_plugin.so' in the gazebo_ros package)");
    return;
  }

  gazebo_ros_ = GazeboRosPtr ( new GazeboRos ( _model, _sdf, "ActorRos" ) );
  gazebo_ros_->isInitialized();

  cmd_vel_subscriber_ = gazebo_ros_->node()->subscribe("cmd_vel", 1, &ActorPluginRos::NewVelCmdCallback, this);
  ROS_INFO_NAMED("ActorRos", "%s: Subscribe to %s", gazebo_ros_->info(), cmd_vel_subscriber_.getTopic().c_str());

  this->sdf = _sdf;
  this->actor = boost::dynamic_pointer_cast<physics::Actor>(_model);
  this->world = this->actor->GetWorld();

  this->connections.push_back(event::Events::ConnectWorldUpdateBegin(
          std::bind(&ActorPluginRos::OnUpdate, this, std::placeholders::_1)));

  this->Reset();

  // Read in the animation factor (applied in the OnUpdate function).
  if (_sdf->HasElement("animation_factor"))
    this->animationFactor = _sdf->Get<double>("animation_factor");
  else
    this->animationFactor = 4.5;
}

/////////////////////////////////////////////////
void ActorPluginRos::Reset()
{
  this->velocity = 0.8;
  this->lastUpdate = 0;

  this->target = ignition::math::Vector3d(0, 0, 1.2138);

  auto skelAnims = this->actor->SkeletonAnimations();
  if (skelAnims.find(WALKING_ANIMATION) == skelAnims.end())
  {
    gzerr << "Skeleton animation " << WALKING_ANIMATION << " not found.\n";
  }
  else
  {
    // Create custom trajectory
    this->trajectoryInfo.reset(new physics::TrajectoryInfo());
    this->trajectoryInfo->type = WALKING_ANIMATION;
    this->trajectoryInfo->duration = 1.0;
    this->last_linear = ignition::math::Vector3d(0,0,0);
    this->last_angle = 0;
    this->actor->SetCustomTrajectory(this->trajectoryInfo);
  }
}

/////////////////////////////////////////////////
void ActorPluginRos::NewVelCmdCallback(const geometry_msgs::Twist::ConstPtr& cmd_msg) {
  double angle = this->actor->WorldPose().Rot().Yaw() * -1;
  this->last_linear = ignition::math::Vector3d(
    cmd_msg->linear.y * cos(angle) - cmd_msg->linear.x * sin(angle),
    -(cmd_msg->linear.y * sin(angle) + cmd_msg->linear.x * cos(angle)),
    0
  );
  this->last_angle = cmd_msg->angular.z;
}

/////////////////////////////////////////////////
void ActorPluginRos::OnUpdate(const common::UpdateInfo &_info)
{
  // Time delta
  double dt = (_info.simTime - this->lastUpdate).Double();

  ignition::math::Pose3d pose = this->actor->WorldPose();
  ignition::math::Vector3d pos = this->target - pose.Pos();
  ignition::math::Vector3d rpy = pose.Rot().Euler();

  double distance = pos.Length();

  ignition::math::Angle yaw = this->last_angle;
  
  // Rotate in place, instead of jumping.
  if (std::abs(yaw.Radian()) > IGN_DTOR(10))
  {
    pose.Rot() = ignition::math::Quaterniond(1.5707, 0, rpy.Z()+
        yaw.Radian()*0.001);
  }
  else
  {
    pose.Pos() += this->last_linear * dt;
    pose.Rot() = ignition::math::Quaterniond(1.5707, 0, rpy.Z()+yaw.Radian());
  }

  // Make sure the actor stays within bounds
  pose.Pos().X(std::max(-10.0, std::min(20.0, pose.Pos().X())));
  pose.Pos().Y(std::max(-20.0, std::min(20.0, pose.Pos().Y())));
  pose.Pos().Z(0.98);

  // Distance traveled is used to coordinate motion with the walking
  // animation
  double distanceTraveled = (pose.Pos() -
      this->actor->WorldPose().Pos()).Length();

  this->actor->SetWorldPose(pose, false, false);
  this->actor->SetScriptTime(this->actor->ScriptTime() +
    (distanceTraveled * this->animationFactor));
  this->lastUpdate = _info.simTime;
}
#include <voxblox_ros/conversions.h>
#include <voxgraph_msgs/MapLayer.h>
#include "voxgraph/tools/map_servers/projected_map_server.h"

namespace voxgraph {
ProjectedMapServer::ProjectedMapServer(ros::NodeHandle nh_private) {
  projected_tsdf_map_pub_ =
      nh_private.advertise<voxgraph_msgs::MapLayer>("projected_map_tsdf",
          1, true);
}

void ProjectedMapServer::publishProjectedMap(
    const VoxgraphSubmapCollection &submap_collection,
    const ros::Time &timestamp) {
  // TODO(victorr): Only publish if there are subscribers
  // Create the message and set its headers
  voxgraph_msgs::MapLayer projected_map_tsdf_msg;
  projected_map_tsdf_msg.header = generateHeaderMsg(timestamp);
  projected_map_tsdf_msg.map_header = generateMapHeaderMsg(submap_collection);

  // Set the message's TSDF
  voxblox::serializeLayerAsMsg<voxblox::TsdfVoxel>(
      submap_collection.getProjectedMap()->getTsdfLayer(), false,
      &projected_map_tsdf_msg.layer);
  projected_map_tsdf_msg.layer.action =
      static_cast<uint8_t>(voxblox::MapDerializationAction::kReset);

  // Publish
  projected_tsdf_map_pub_.publish(projected_map_tsdf_msg);
}

std_msgs::Header ProjectedMapServer::generateHeaderMsg(
    const ros::Time &timestamp) {
  std_msgs::Header msg_header;
  // TODO(victorr): Get the world frame name from FrameNames once implemented
  msg_header.frame_id = "mission";
  msg_header.stamp = timestamp;
  return msg_header;
}

voxgraph_msgs::MapHeader ProjectedMapServer::generateMapHeaderMsg(
    const VoxgraphSubmapCollection &submap_collection) {
  // Set the map ID and type
  voxgraph_msgs::MapHeader map_header;
  map_header.id = 0;
  map_header.is_submap = false;

  // Set the map's start and end time
  if (!submap_collection.empty()) {
    map_header.start_time =
        submap_collection.getSubmap(
            submap_collection.getFirstSubmapId()).getStartTime();
    map_header.end_time =
        submap_collection.getSubmap(
            submap_collection.getLastSubmapId()).getEndTime();
  } else {
    map_header.start_time = ros::Time(0.0);
    map_header.end_time = ros::Time(0.0);
  }

  // Set the pose estimate to zero
  // TODO(victorr): Get the world frame name from FrameNames once implemented
  map_header.pose_estimate.frame_id = "mission";
  tf::poseKindrToMsg(Transformation().cast<double>(),
                     &map_header.pose_estimate.map_pose);

  return map_header;
}
}  // namespace voxgraph

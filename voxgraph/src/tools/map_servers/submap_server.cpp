#include <minkindr_conversions/kindr_msg.h>
#include <voxblox_ros/conversions.h>
#include <pcl_conversions/pcl_conversions.h>
#include <voxgraph_msgs/MapLayer.h>
#include <voxgraph_msgs/MapSurface.h>
#include "voxgraph/tools/map_servers/submap_server.h"

namespace voxgraph {
SubmapServer::SubmapServer(ros::NodeHandle nh_private) {
  submap_tsdf_pub_ =
      nh_private.advertise<voxgraph_msgs::MapLayer>("submap_tsdfs", 3, false);
  submap_esdf_pub_ =
      nh_private.advertise<voxgraph_msgs::MapLayer>("submap_esdfs", 3, false);
  submap_surface_pointcloud_pub_ =
      nh_private.advertise<sensor_msgs::PointCloud2>(
          "submap_surface_pointclouds", 3, false);
}

void SubmapServer::publishSubmap(const VoxgraphSubmap &submap,
                                 const ros::Time &timestamp) {
  // TODO(victorr): Only publish if there are subscribers
  publishSubmapTsdf(submap, timestamp);
  publishSubmapEsdf(submap, timestamp);
  publishSubmapSurfacePointcloud(submap, timestamp);
}

void SubmapServer::publishSubmapTsdf(const VoxgraphSubmap &submap,
                                     const ros::Time &timestamp) {
  // Create the message and set its headers
  voxgraph_msgs::MapLayer submap_tsdf_msg;
  submap_tsdf_msg.header = generateHeaderMsg(submap, timestamp);
  submap_tsdf_msg.map_header = generateSubmapHeaderMsg(submap);

  // Set the message's TSDF
  voxblox::serializeLayerAsMsg<voxblox::TsdfVoxel>(
      submap.getTsdfMap().getTsdfLayer(), false, &submap_tsdf_msg.layer);
  submap_tsdf_msg.layer.action =
      static_cast<uint8_t>(voxblox::MapDerializationAction::kReset);

  // Publish
  submap_tsdf_pub_.publish(submap_tsdf_msg);
}

void SubmapServer::publishSubmapEsdf(const VoxgraphSubmap &submap,
                                     const ros::Time &timestamp) {
  // Create the message and set its headers
  voxgraph_msgs::MapLayer submap_esdf_msg;
  submap_esdf_msg.header = generateHeaderMsg(submap, timestamp);
  submap_esdf_msg.map_header = generateSubmapHeaderMsg(submap);

  // Set the message's ESDF
  voxblox::serializeLayerAsMsg<voxblox::EsdfVoxel>(
      submap.getEsdfMap().getEsdfLayer(), false, &submap_esdf_msg.layer);
  submap_esdf_msg.layer.action =
      static_cast<uint8_t>(voxblox::MapDerializationAction::kReset);

  // Publish
  submap_esdf_pub_.publish(submap_esdf_msg);
}


void SubmapServer::publishSubmapSurfacePointcloud(const VoxgraphSubmap &submap,
                                                  const ros::Time &timestamp) {
  // Create the message and set its headers
  voxgraph_msgs::MapSurface submap_surface_pointcloud_msg;
  submap_surface_pointcloud_msg.header = generateHeaderMsg(submap, timestamp);
  submap_surface_pointcloud_msg.map_header = generateSubmapHeaderMsg(submap);

  // Fill the message's pointcloud with the vertices of the submap's isosurface
  // TODO(victorr): Implement weighted subsampling for low bandwidth scenarios
  const WeightedSampler<RegistrationPoint> &isosurface_points =
      submap.getRegistrationPoints(
          VoxgraphSubmap::RegistrationPointType::kIsosurfacePoints);
  pcl::PointCloud<pcl::PointXYZI> pcl_surface_pointcloud;
  pcl_surface_pointcloud.reserve(isosurface_points.size());
  for (size_t point_i = 0; point_i < isosurface_points.size(); point_i++) {
    pcl::PointXYZI pcl_isosurface_point;
    pcl_isosurface_point.x = isosurface_points[point_i].position.x();
    pcl_isosurface_point.y = isosurface_points[point_i].position.y();
    pcl_isosurface_point.z = isosurface_points[point_i].position.z();
    pcl_isosurface_point.intensity = isosurface_points[point_i].weight;
    pcl_surface_pointcloud.push_back(pcl_isosurface_point);
  }
  pcl::toROSMsg(pcl_surface_pointcloud, submap_surface_pointcloud_msg.pointcloud);

  // Publish
  submap_surface_pointcloud_pub_.publish(submap_surface_pointcloud_msg);
}

std_msgs::Header SubmapServer::generateHeaderMsg(const VoxgraphSubmap &submap,
                                                 const ros::Time &timestamp) {
  std_msgs::Header msg_header;
  msg_header.frame_id = "submap_" + std::to_string(submap.getID());
  msg_header.stamp = timestamp;
  return msg_header;
}

voxgraph_msgs::MapHeader SubmapServer::generateSubmapHeaderMsg(
    const VoxgraphSubmap &submap) {
  // Set the submap ID and type
  voxgraph_msgs::MapHeader submap_header;
  submap_header.id = submap.getID();
  submap_header.is_submap = true;

  // Set the submap's start and end time
  const VoxgraphSubmap::PoseHistoryMap &pose_history =
      submap.getPoseHistory();
  if (!pose_history.empty()) {
    submap_header.start_time = pose_history.begin()->first;
    submap_header.end_time = (--pose_history.end())->first;
  } else {
    submap_header.start_time = ros::Time(0.0);
    submap_header.end_time = ros::Time(0.0);
  }

  // Set the pose estimate and indicate what frame it's in
  // TODO(victorr): Get the world frame name from FrameNames once implemented
  submap_header.pose_estimate.frame_id = "mission";
  tf::poseKindrToMsg(submap.getPose().cast<double>(),
                     &submap_header.pose_estimate.map_pose);

  return submap_header;
}
}  // namespace voxgraph

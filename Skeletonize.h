#ifndef IMAGEPROCESSING_TUBES_SKELETONIZE_H__
#define IMAGEPROCESSING_TUBES_SKELETONIZE_H__

#include <imageprocessing/ExplicitVolume.h>
#define WITH_LEMON
#include <lemon/dijkstra.h>
#include "Skeleton.h"

class NoNodeFound : public Exception {};

class Skeletonize {

	typedef vigra::MultiArray<3, unsigned char> VolumeType;
	typedef VolumeType::difference_type         Position;
	typedef GraphVolume::Graph::EdgeMap<double> DistanceMap;

public:

	/**
	 * Create a skeletonizer for the given volume. Inside voxels are assumed to 
	 * be labelled with 1, background with 0.
	 */
	Skeletonize(const GraphVolume& graph);

	/**
	 * Extract the skeleton from the given volume.
	 */
	Skeleton getSkeleton();

private:

	enum VoxelLabel {

		Background = 0,
		Inside     = 1, /* ordinary inside voxels and initial values */
		Boundary   = 2, /* inside voxels on boundary */
		Explained  = 3, /* boundary voxels that are within a threshold distance to skeleton voxels */
		OnSkeleton = 4, /* skeleton voxels */
		Visited    = 5  /* skeleton voxels that have been added to Skeleton datastructure (eventually, all OnSkeleton voxels)*/
	};

	/**
	 * Create a list of all boundary nodes in the graph volume.
	 */
	void findBoundaryNodes();

	/**
	 * Initialize the edge map, such that initial edges inside the volume are 
	 * Euclidean distance plus boundary distance penalty, and all other ones 
	 * infinite.
	 */
	void initializeEdgeMap();

	/**
	 * Find the root node as the furthest point from the highest boundary 
	 * distance point.
	 */
	void findRoot();

	/**
	 * Compute or update the shortest paths from the root node to all other 
	 * points. Takes the current edge map for distances.
	 */
	void findShortestPaths();

	/**
	 * Find the furthest point and walk backwards along the shortest path to the 
	 * current skeleton. This marks all points on the path as being part of the 
	 * skeleton, and all points in the vicinity as beeing processed. The edge 
	 * values along the shortest path will be set to zero. Additionally, the 
	 * segment will be added to the passed skeleton.
	 *
	 * Returns true, if a path that is far enough from the existing skeleton was 
	 * found.
	 */
	bool extractLongestSegment();

	/**
	 * Draw a sphere around the current point, marking all boundary points 
	 * within it as explained.
	 */
	void drawExplanationSphere(const Position& center);

	/**
	 * Convert grid positions to volume positions.
	 */
	Skeleton::Position gridToVolume(Position pos) {

		return Skeleton::Position(
				_graphVolume.getBoundingBox().min().x() + (float)pos[0]*_graphVolume.getResolutionX(),
				_graphVolume.getBoundingBox().min().y() + (float)pos[1]*_graphVolume.getResolutionY(),
				_graphVolume.getBoundingBox().min().z() + (float)pos[2]*_graphVolume.getResolutionZ());
	}

	/**
	 * Compute the boundary penalty term.
	 */
	double boundaryPenalty(double boundaryDistance);

	/**
	 * Extract a Skeleton from the annotated volume.
	 */
	Skeleton parseVolumeSkeleton();

	/**
	 * Recursively discover the skeleton graph from the volume annotations.
	 */
	void traverse(const Position& pos, Skeleton& skeleton);

	/**
	 * The number of neighbors of a skeleton position in the volume.
	 */
	int numNeighbors(const Position& pos);

	// copy of the volume to process, and interior boundary distances
	vigra::MultiArray<3, unsigned char> _volume;
	vigra::MultiArray<3, float>         _boundaryDistance;

	// lemon graph compatible datastructures for Dijkstra
	const GraphVolume& _graphVolume;
	DistanceMap _distanceMap;

	double _boundaryWeight;

	lemon::Dijkstra<GraphVolume::Graph, DistanceMap> _dijkstra;

	GraphVolume::Graph::Node _root;
	GraphVolume::Graph::Node _center;

	std::vector<GraphVolume::Graph::Node> _boundary;

	float _maxBoundaryDistance2;

	double _minSegmentLength;
	double _minSegmentLengthRatio;

	bool   _skipExplainedNodes;
	double _explanationWeight;
};

#endif // IMAGEPROCESSING_TUBES_SKELETONIZE_H__


#pragma once

// only included in case there's a C++11 compiler out there that doesn't support `#pragma once`
#ifndef DKM_PARALLEL_KMEANS_H
#define DKM_PARALLEL_KMEANS_H

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <random>
#include <tuple>
#include <type_traits>
#include <vector>

#include "dkm.hpp"

/*
DKM - A k-means implementation that is generic across variable data dimensions.
*/
namespace dkm {

/*
These functions are all private implementation details and shouldn't be referenced outside of this
file.
*/
namespace details {

/*
Calculate the smallest distance between each of the data points and any of the input means.
*/
template <typename T, size_t N>
std::vector<T> closest_distance_parallel(
	const std::vector<std::array<T, N>>& means, const std::vector<std::array<T, N>>& data) {
	std::vector<T> distances(data.size(), T());
	#pragma omp parallel for
	for (int i = 0; i < (int)data.size(); ++i) {
		T closest = distance_squared(data[i], means[0]);
		for (const auto& m : means) {
			T distance = distance_squared(data[i], m);
			if (distance < closest)
				closest = distance;
		}
		distances[i] = closest;
	}
	return distances;
}

/*
This is an alternate initialization method based on the [kmeans++](https://en.wikipedia.org/wiki/K-means%2B%2B)
initialization algorithm.
*/
template <typename T, size_t N>
std::vector<std::array<T, N>> random_plusplus_parallel(const std::vector<std::array<T, N>>& data, uint32_t k, uint64_t seed) {
	assert(k > 0);
	assert(data.size() > 0);
	using input_size_t = typename std::array<T, N>::size_type;
	std::vector<std::array<T, N>> means;
	// Using a very simple PRBS generator, parameters selected according to
	// https://en.wikipedia.org/wiki/Linear_congruential_generator#Parameters_in_common_use
	std::linear_congruential_engine<uint64_t, 6364136223846793005, 1442695040888963407, UINT64_MAX> rand_engine(seed);

	// Select first mean at random from the set
	{
		std::uniform_int_distribution<input_size_t> uniform_generator(0, data.size() - 1);
		means.push_back(data[uniform_generator(rand_engine)]);
	}

	for (uint32_t count = 1; count < k; ++count) {
		// Calculate the distance to the closest mean for each data point
		auto distances = details::closest_distance_parallel(means, data);
		// Pick a random point weighted by the distance from existing means
		// TODO: This might convert floating point weights to ints, distorting the distribution for small weights
#if !defined(_MSC_VER) || _MSC_VER >= 1900
		std::discrete_distribution<input_size_t> generator(distances.begin(), distances.end());
#else  // MSVC++ older than 14.0
		input_size_t i = 0;
		std::discrete_distribution<input_size_t> generator(distances.size(), 0.0, 0.0, [&distances, &i](double) { return distances[i++]; });
#endif
		means.push_back(data[generator(rand_engine)]);
	}
	return means;
}

/*
Calculate the index of the mean each data point is closest to (euclidean distance).
*/
template <typename T, size_t N>
std::vector<uint32_t> calculate_clusters_parallel(
	const std::vector<std::array<T, N>>& data, const std::vector<std::array<T, N>>& means) {
	std::vector<uint32_t> clusters(data.size(), 0);
	#pragma omp parallel for
	for (int i = 0; i < (int)data.size(); ++i) {
		clusters[i] = closest_mean(data[i], means);
	}
	return clusters;
}

} // namespace details


/*
Implementation of k-means generic across the data type and the dimension of each data item. Expects
the data to be a vector of fixed-size arrays. Generic parameters are the type of the base data (T)
and the dimensionality of each data point (N). All points must have the same dimensionality.

e.g. points of the form (X, Y, Z) would be N = 3.

Returns a std::tuple containing:
  0: A vector holding the means for each cluster from 0 to k-1.
  1: A vector containing the cluster number (0 to k-1) for each corresponding element of the input
	 data vector.

Implementation details:
This implementation of k-means uses [Lloyd's Algorithm](https://en.wikipedia.org/wiki/Lloyd%27s_algorithm)
with the [kmeans++](https://en.wikipedia.org/wiki/K-means%2B%2B)
used for initializing the means.
*/
template <typename T, size_t N>
std::tuple<std::vector<std::array<T, N>>, std::vector<uint32_t>> kmeans_lloyd_parallel(
	const std::vector<std::array<T, N>>& data, const clustering_parameters<T>& parameters) {
	static_assert(std::is_arithmetic<T>::value && std::is_signed<T>::value,
		"kmeans_lloyd requires the template parameter T to be a signed arithmetic type (e.g. float, double, int)");
	assert(parameters.get_k() > 0); // k must be greater than zero
	assert(data.size() >= parameters.get_k()); // there must be at least k data points
	std::random_device rand_device;
	uint64_t seed = parameters.has_random_seed() ? parameters.get_random_seed() : rand_device();
	std::vector<std::array<T, N>> means = details::random_plusplus_parallel(data, parameters.get_k(), seed);

	std::vector<std::array<T, N>> old_means;
	std::vector<std::array<T, N>> old_old_means;
	std::vector<uint32_t> clusters;
	// Calculate new means until convergence is reached or we hit the maximum iteration count
	uint64_t count = 0;
	do {
		clusters = details::calculate_clusters_parallel(data, means);
		old_old_means = old_means;
		old_means = means;
		means = details::calculate_means(data, clusters, old_means, parameters.get_k());
		++count;
	} while ((means != old_means && means != old_old_means)
		&& !(parameters.has_max_iteration() && count == parameters.get_max_iteration())
		&& !(parameters.has_min_delta() && details::deltas_below_limit(details::deltas(old_means, means), parameters.get_min_delta())));

	return std::tuple<std::vector<std::array<T, N>>, std::vector<uint32_t>>(means, clusters);
}

/*
This overload exists to support legacy code which uses this signature of the kmeans_lloyd function.
Any code still using this signature should move to the version of this function that uses a
`clustering_parameters` struct for configuration.
*/
template <typename T, size_t N>
std::tuple<std::vector<std::array<T, N>>, std::vector<uint32_t>> kmeans_lloyd_parallel(
	const std::vector<std::array<T, N>>& data, uint32_t k,
	uint64_t max_iter = 0, T min_delta = -1.0) {
	clustering_parameters<T> parameters(k);
	if (max_iter != 0) {
		parameters.set_max_iteration(max_iter);
	}
	if (min_delta != 0) {
		parameters.set_min_delta(min_delta);
	}
	return kmeans_lloyd_parallel(data, parameters);
}

} // namespace dkm

#endif /* DKM_PARALLEL_KMEANS_H */

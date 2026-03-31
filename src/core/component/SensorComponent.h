#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "core/component/IComponent.h"
#include "core/entity/Entity.h"

namespace fm::core {

class SensorComponent final : public IComponent {
public:
    struct RadarParameters {
        double peakTransmitPowerWatts {0.0};
        double antennaGainDecibels {0.0};
        double centerFrequencyHertz {0.0};
        double signalBandwidthHertz {0.0};
        double noiseFigureDecibels {0.0};
        double systemLossDecibels {0.0};
        double requiredSnrDecibels {13.0};
        double processingGainDecibels {0.0};
        double scanRateHertz {0.0};
        double receiverTemperatureKelvin {290.0};
    };

    explicit SensorComponent(double rangeMeters, double fieldOfViewDegrees = 360.0)
        : rangeMeters_(rangeMeters), fieldOfViewDegrees_(fieldOfViewDegrees)
    {
    }

    SensorComponent(double rangeMeters, double fieldOfViewDegrees, RadarParameters radarParameters)
        : rangeMeters_(rangeMeters)
        , fieldOfViewDegrees_(fieldOfViewDegrees)
        , radarParameters_(std::move(radarParameters))
    {
    }

    UpdatePhase phase() const override
    {
        return UpdatePhase::Perception;
    }

    void update(const SimulationUpdateContext& context) override
    {
        if (radarParameters_.scanRateHertz > 0.0) {
            accumulatedScanTimeSeconds_ += context.deltaSeconds;
            const double scanPeriodSeconds = 1.0 / radarParameters_.scanRateHertz;
            if (accumulatedScanTimeSeconds_ + 1e-9 < scanPeriodSeconds) {
                return;
            }
            accumulatedScanTimeSeconds_ = std::fmod(accumulatedScanTimeSeconds_, scanPeriodSeconds);
        }

        detectedTargetIds_.clear();
        if (rangeMeters_ <= 0.0 && !hasDetailedRadarModel()) {
            return;
        }

        const auto rangeSquared = rangeMeters_ * rangeMeters_;
        const double clampedFieldOfViewDegrees = std::clamp(fieldOfViewDegrees_, 0.0, 360.0);
        const bool useFieldOfViewFilter = clampedFieldOfViewDegrees < 359.999;
        const double halfFieldOfViewRadians = clampedFieldOfViewDegrees * 0.5 * kDegreesToRadians;
        const double minimumMagnitudeSquared = 1e-12;
        const double speedSquared = owner().velocity().x * owner().velocity().x + owner().velocity().y * owner().velocity().y;
        const double headingRadians = owner().headingDegrees() * kDegreesToRadians;
        const double forwardX = speedSquared > minimumMagnitudeSquared
            ? owner().velocity().x / std::sqrt(speedSquared)
            : std::cos(headingRadians);
        const double forwardY = speedSquared > minimumMagnitudeSquared
            ? owner().velocity().y / std::sqrt(speedSquared)
            : std::sin(headingRadians);

        for (const auto& entity : context.entities) {
            if (entity == nullptr || entity.get() == &owner()) {
                continue;
            }

            const bool sameKnownSide = !owner().side().empty()
                && !entity->side().empty()
                && entity->side() == owner().side();
            if (sameKnownSide) {
                continue;
            }

            const auto dx = entity->position().x - owner().position().x;
            const auto dy = entity->position().y - owner().position().y;
            const auto distanceSquared = dx * dx + dy * dy;
            if (rangeMeters_ > 0.0 && distanceSquared > rangeSquared) {
                continue;
            }

            if (useFieldOfViewFilter && distanceSquared > minimumMagnitudeSquared) {
                const double distance = std::sqrt(distanceSquared);
                const double targetDirectionX = dx / distance;
                const double targetDirectionY = dy / distance;
                const double dotProduct = std::clamp(forwardX * targetDirectionX + forwardY * targetDirectionY, -1.0, 1.0);
                const double angleOffsetRadians = std::acos(dotProduct);
                if (angleOffsetRadians > halfFieldOfViewRadians) {
                    continue;
                }
            }

            if (hasDetailedRadarModel()) {
                const double snrDecibels = estimateSignalToNoiseRatioDecibels(std::sqrt(distanceSquared),
                                                                              entity->radarCrossSectionSquareMeters());
                if (snrDecibels + 1e-9 < radarParameters_.requiredSnrDecibels) {
                    continue;
                }
            }

            detectedTargetIds_.push_back(entity->id());
        }
    }

    double rangeMeters() const
    {
        if (rangeMeters_ > 0.0) {
            return rangeMeters_;
        }

        if (hasDetailedRadarModel()) {
            return estimateMaximumRangeMetersForRcs(1.0);
        }

        return rangeMeters_;
    }

    double fieldOfViewDegrees() const
    {
        return fieldOfViewDegrees_;
    }

    const std::vector<std::string>& detectedTargetIds() const
    {
        return detectedTargetIds_;
    }

    double estimateSignalToNoiseRatioDecibels(double rangeMeters, double radarCrossSectionSquareMeters) const
    {
        if (!hasDetailedRadarModel() || rangeMeters <= 0.0 || radarCrossSectionSquareMeters <= 0.0) {
            return -std::numeric_limits<double>::infinity();
        }

        const double wavelengthMeters = kSpeedOfLightMetersPerSecond / radarParameters_.centerFrequencyHertz;
        const double antennaGainLinear = decibelsToLinear(radarParameters_.antennaGainDecibels);
        const double systemLossLinear = decibelsToLinear(radarParameters_.systemLossDecibels);
        const double processingGainLinear = decibelsToLinear(radarParameters_.processingGainDecibels);
        const double receivedPowerWatts = radarParameters_.peakTransmitPowerWatts
            * antennaGainLinear * antennaGainLinear
            * wavelengthMeters * wavelengthMeters
            * radarCrossSectionSquareMeters
            / (std::pow(4.0 * kPi, 3.0) * std::pow(rangeMeters, 4.0) * systemLossLinear);
        const double noisePowerWatts = kBoltzmannConstant
            * std::max(1.0, radarParameters_.receiverTemperatureKelvin)
            * radarParameters_.signalBandwidthHertz
            * decibelsToLinear(radarParameters_.noiseFigureDecibels);
        if (receivedPowerWatts <= 0.0 || noisePowerWatts <= 0.0) {
            return -std::numeric_limits<double>::infinity();
        }

        return linearToDecibels((receivedPowerWatts * processingGainLinear) / noisePowerWatts);
    }

    double estimateMaximumRangeMetersForRcs(double radarCrossSectionSquareMeters) const
    {
        if (!hasDetailedRadarModel() || radarCrossSectionSquareMeters <= 0.0) {
            return 0.0;
        }

        const double wavelengthMeters = kSpeedOfLightMetersPerSecond / radarParameters_.centerFrequencyHertz;
        const double antennaGainLinear = decibelsToLinear(radarParameters_.antennaGainDecibels);
        const double thresholdSnrLinear = decibelsToLinear(
            std::max(0.0, radarParameters_.requiredSnrDecibels - radarParameters_.processingGainDecibels));
        const double noisePowerWatts = kBoltzmannConstant
            * std::max(1.0, radarParameters_.receiverTemperatureKelvin)
            * radarParameters_.signalBandwidthHertz
            * decibelsToLinear(radarParameters_.noiseFigureDecibels);
        const double numerator = radarParameters_.peakTransmitPowerWatts
            * antennaGainLinear * antennaGainLinear
            * wavelengthMeters * wavelengthMeters
            * radarCrossSectionSquareMeters;
        const double denominator = std::pow(4.0 * kPi, 3.0)
            * decibelsToLinear(radarParameters_.systemLossDecibels)
            * noisePowerWatts
            * thresholdSnrLinear;
        if (numerator <= 0.0 || denominator <= 0.0) {
            return 0.0;
        }

        return std::pow(numerator / denominator, 0.25);
    }

private:
    static constexpr double kDegreesToRadians = 3.14159265358979323846 / 180.0;
    static constexpr double kPi = 3.14159265358979323846;
    static constexpr double kBoltzmannConstant = 1.380649e-23;
    static constexpr double kSpeedOfLightMetersPerSecond = 299792458.0;

    static double decibelsToLinear(double value)
    {
        return std::pow(10.0, value / 10.0);
    }

    static double linearToDecibels(double value)
    {
        return 10.0 * std::log10(std::max(value, 1e-30));
    }

    bool hasDetailedRadarModel() const
    {
        return radarParameters_.peakTransmitPowerWatts > 0.0
            && radarParameters_.antennaGainDecibels > 0.0
            && radarParameters_.centerFrequencyHertz > 0.0
            && radarParameters_.signalBandwidthHertz > 0.0;
    }

    double rangeMeters_ {0.0};
    double fieldOfViewDegrees_ {360.0};
    RadarParameters radarParameters_;
    double accumulatedScanTimeSeconds_ {0.0};
    std::vector<std::string> detectedTargetIds_;
};

}  // namespace fm::core
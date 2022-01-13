#include "inivation.hpp"
#include "inivation_device.hpp"

CAERUSBConnection::CAERUSBConnection(std::string camera, uint16_t deviceId,
                                 uint8_t deviceAddress) {

  try {
    if (camera == "dvx") {
      handle = new libcaer::devices::dvXplorer(deviceId, deviceId,
                                               deviceAddress, "");
    } else if (camera == "davis") {
      handle =
          new libcaer::devices::davis(deviceId, deviceId, deviceAddress, "");
    } else {
      throw std::invalid_argument("Unknown camera '" + camera + "'");
    }
  } catch (const std::exception &e) {
    std::cout << "Failure with camera: " << camera << ", deviceId: '"
              << deviceId << "', deviceAddress (devAddress) see below.\n"
              << std::endl;
    std::cout << e.what() << std::endl;

    std::cout << "List of available inivation cameras and configurations: "
              << std::endl;

    int exit_status = list_devices();

    std::cout << std::endl;

    throw std::invalid_argument(
        "Please choose the deviceId (busNum) and deviceAddress (devAddr) of "
        "one of the above listed iniVation cameras!");
  }

  // Send the default configuration before using the device.
  // No configuration is sent automatically!
  handle->sendDefaultConfig();

  // Set parsing intervall where container interval is in [10μs] unit
  // davisHandle.configSet(CAER_HOST_CONFIG_PACKETS,
  // CAER_HOST_CONFIG_PACKETS_MAX_CONTAINER_INTERVAL, container_interval);

  // Set number of events per packet
  handle->configSet(CAER_HOST_CONFIG_PACKETS,
                    CAER_HOST_CONFIG_PACKETS_MAX_CONTAINER_PACKET_SIZE,
                    containerInterval);

  // Configs about buffer
  handle->configSet(CAER_HOST_CONFIG_DATAEXCHANGE,
                    CAER_HOST_CONFIG_DATAEXCHANGE_BUFFER_SIZE, bufferSize);

  // Start data stream
  handle->dataStart(nullptr, nullptr, nullptr, &shutdownHandler, nullptr);

  // Let's turn on blocking data-get mode to avoid wasting resources.
  handle->configSet(CAER_HOST_CONFIG_DATAEXCHANGE,
                    CAER_HOST_CONFIG_DATAEXCHANGE_BLOCKING, true);
}

// event generator for Inivation cameras
Generator<AEDAT::PolarityEvent>
inivation_event_generator(std::string camera, std::uint16_t deviceId,
                    std::uint8_t deviceAddress) {

  auto connection = CAERUSBConnection(camera, deviceId, deviceAddress);

  std::unique_ptr<libcaer::events::EventPacketContainer> packetContainer;
  while (true) {
    do {
      packetContainer = connection.getPacket();
    } while (packetContainer == nullptr);

    for (auto &packet : *packetContainer) {
      if (packet == nullptr) {
        continue; // Skip if nothing there.
      }

      if (packet->getEventType() == POLARITY_EVENT) {
        std::shared_ptr<const libcaer::events::PolarityEventPacket> polarity =
            std::static_pointer_cast<libcaer::events::PolarityEventPacket>(
                packet);

        for (const libcaer::events::PolarityEvent &evt : *polarity) {
          if (!evt.isValid()) {
            continue;
          }

          const AEDAT::PolarityEvent polarityEvent = {
              (uint64_t)evt.getTimestamp64(*polarity),
              evt.getX(),
              evt.getY(),
              evt.isValid(),
              evt.getPolarity(),
          };

          co_yield polarityEvent;
        }
      }
    }
  }
};
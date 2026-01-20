// SPDX-FileCopyrightText: 2025 Contributors to the Media eXchange Layer project.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>
#include <unistd.h>
#include <uuid.h>
#include <mxl/platform.h>

namespace mxl::lib
{

    class DiscreteFlowWriter;

    /// Entry stored in the unordered_maps
    struct DomainWatcherRecord
    {
        typedef std::shared_ptr<DomainWatcherRecord> ptr;

        /// flow id
        uuids::uuid id;
        /// file being watched
        std::string fileName;

        DiscreteFlowWriter* fw;

        [[nodiscard]]
        bool operator==(DomainWatcherRecord const& other) const noexcept
        {
            return id == other.id && fw == other.fw;
        }
    };

    ///
    /// Monitors flows on disk for changes.
    ///
    /// A flow reader is looking for changes to the {mxl_domain}/{flow_id}.mxl-flow/data file
    /// An update to this flow triggers a callback that will notify a condition variable in the relevant FlowReaders.
    /// if a reader is waiting for the next grain it will be notified that the grain is ready.
    ///
    /// A flow writer is looking for changes to the {mxl_domain}/{flow_id}.mxl-flow/access file. This file is 'touched'
    /// by readers when they read a grain, which will trigger a 'FlowInfo.lastRead` update (performed by the FlowWriter
    /// since the writer is the only one mapping the flow in write-mode).
    ///
    class MXL_EXPORT DomainWatcher
    {
    public:
        typedef std::shared_ptr<DomainWatcher> ptr;

        ///
        /// Constructor that initializes inotify and epoll/kqueue, and starts the event processing thread.
        /// \param in_domain The mxl domain path to monitor.
        /// \param in_callback Function to be called when a monitored file's attributes change.
        ///
        explicit DomainWatcher(std::filesystem::path const& in_domain);

        ///
        /// Destructor that stops the event loop, removes all watches, and closes file descriptors.
        ///
        ~DomainWatcher();

        void addFlow(DiscreteFlowWriter* writer, uuids::uuid id);
        void removeFlow(DiscreteFlowWriter* writer, uuids::uuid id);

        ///
        /// Stops the running thread
        ///
        void stop()
        {
            _running = false;
        }

        std::size_t count(uuids::uuid) noexcept;
        std::size_t size() noexcept;

    private:
        void addRecord(DomainWatcherRecord record);
        void removeRecord(DomainWatcherRecord const& record);

        /// Event loop that waits for inotify file change events and processes them.
        /// (invokes the callback)
        void processEvents();
        /// The monitored domain
        std::filesystem::path _domain;

#ifdef __APPLE__
        int _kq;
#elif defined __linux__
        /// The inotify file descriptor
        int _inotifyFd;
        /// The epoll fd monitoring inotify
        int _epollFd;
#endif

        /// Map of watch descriptors to file records.  Multiple records could use the same watchfd
        std::unordered_multimap<int, DomainWatcherRecord> _watches;
        /// Prodect maps
        std::mutex _mutex;
        /// Controls the event processing thread
        std::atomic<bool> _running;
        /// Event processing thread
        std::thread _watchThread;
    };

} // namespace mxl::lib

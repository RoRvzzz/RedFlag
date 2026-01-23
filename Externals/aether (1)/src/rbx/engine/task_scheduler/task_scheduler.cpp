#include <windows.h>
#include <TlHelp32.h>
#include <string>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <wininet.h>
#include <ShlObj.h>

#include <vector>
#include "../../main.h"

#include "../../../misc/Umodule/Umodule.hpp"
#include "../Classes/offsets/offsets.hpp"
#include "../../../misc/output_system/output/output.hpp"

uintptr_t RBX::TaskScheduler::GetScheduler() {
    utils::output::info("Fetching process base for scheduler...");
    virtualaddy = Umodule::GetProcessBase();

    if (!virtualaddy) {
        utils::output::warning("Process base (module base) is NULL.");
        return 0;
    }

    utils::output::info("Reading TaskScheduler pointer from base: " + utils::format_hex(virtualaddy));
    uintptr_t result = Umodule::read<uintptr_t>(virtualaddy + Offsets::TaskScheduler::Pointer);

    utils::output::print("TaskScheduler pointer resolved: " + utils::format_hex(result));
    return result;
}

std::vector<RBX::Instance> RBX::TaskScheduler::GetJobs() {
    utils::output::info("Fetching all jobs from TaskScheduler...");
    std::vector<RBX::Instance> jobs;
    uintptr_t scheduler = GetScheduler();

    if (!scheduler) {
        utils::output::error("GetScheduler returned NULL. No jobs can be fetched.");
        return jobs;
    }

    uintptr_t jobStart = Umodule::read<uintptr_t>(scheduler + Offsets::TaskScheduler::JobStart);
    uintptr_t jobEnd = Umodule::read<uintptr_t>(scheduler + Offsets::TaskScheduler::JobEnd);

    utils::output::info("JobStart: " + utils::format_hex(jobStart));
    utils::output::info("JobEnd: " + utils::format_hex(jobEnd));

    if (jobStart && jobEnd && jobStart < jobEnd) {
        for (uintptr_t job = jobStart; job < jobEnd; job += Offsets::TaskScheduler::JobInterpation) {
            uintptr_t jobAddress = Umodule::read<uintptr_t>(job);
            if (jobAddress) {
                utils::output::print("Discovered Job Address: " + utils::format_hex(jobAddress));
                jobs.emplace_back(jobAddress);
            }
            else {
                utils::output::info("Skipped NULL job at: " + utils::format_hex(job));
            }
        }
    }
    else {
        utils::output::warning("Invalid jobStart/jobEnd values. Skipping job collection.");
    }

    utils::output::info("Total jobs collected: " + std::to_string(jobs.size()));
    return jobs;
}

std::string RBX::TaskScheduler::GetJobName(RBX::Instance instance) {
    utils::output::info("Fetching job name at: " + utils::format_hex(instance.address));
    std::string jobName = RBX::Instance::ReadString(instance.address + Offsets::TaskScheduler::JobName);
    utils::output::print("Resolved Job Name: " + jobName);
    return jobName;
}

uintptr_t RBX::TaskScheduler::GetJobByName(const std::string& tarGetName) {
    utils::output::info("Searching for job by name: " + tarGetName);
    uintptr_t scheduler = GetScheduler();
    if (!scheduler) {
        utils::output::error("GetScheduler returned NULL.");
        return 0;
    }

    uintptr_t jobStart = Umodule::read<uintptr_t>(scheduler + Offsets::TaskScheduler::JobStart);
    uintptr_t jobEnd = Umodule::read<uintptr_t>(scheduler + Offsets::TaskScheduler::JobEnd);

    utils::output::info("Job scan range: " + utils::format_hex(jobStart) + " - " + utils::format_hex(jobEnd));

    for (uintptr_t i = jobStart; i < jobEnd; i += Offsets::TaskScheduler::JobInterpation) {
        uintptr_t jobAddr = Umodule::read<uintptr_t>(i);
        if (!jobAddr) {
            utils::output::info("Null job pointer at: " + utils::format_hex(i));
            continue;
        }

        std::string jobName = RBX::Instance::ReadString(jobAddr + Offsets::TaskScheduler::JobName);
        utils::output::info("Job Found: " + jobName + " at " + utils::format_hex(jobAddr));

        if (jobName.find(tarGetName) != std::string::npos) {
            utils::output::info("Match found: " + jobName);
            return jobAddr;
        }
    }

    utils::output::warning("No matching job found for: " + tarGetName);
    return 0;
}

uintptr_t RBX::TaskScheduler::GetTargetFPS() {
    uintptr_t scheduler = GetScheduler();
    double rawFps = Umodule::read<double>(scheduler + Offsets::TaskScheduler::CurrentFps);
    utils::output::info("Raw FPS value: " + std::to_string(rawFps));
    return static_cast<uintptr_t>(1.0 / rawFps);
}

uintptr_t RBX::TaskScheduler::SetTargetFPS(double value) {
    uintptr_t scheduler = GetScheduler();
    double toWrite = 1.0 / value;
    utils::output::info("Setting Target FPS to: " + std::to_string(value) + " (raw = " + std::to_string(toWrite) + ")");
    return Umodule::write<double>(scheduler + Offsets::TaskScheduler::CurrentFps, toWrite);
}

void RBX::TaskScheduler::PauseTask(uintptr_t jobAddress) {
    if (jobAddress) {
        utils::output::info("Pausing job at address: " + utils::format_hex(jobAddress));
        Umodule::write<bool>(jobAddress + Offsets::TaskScheduler::JobBuisnessPlaying, true);
    }
    else {
        utils::output::warning("PauseTask called with null job address.");
    }
}

void RBX::TaskScheduler::ResumeTask(uintptr_t jobAddress) {
    if (jobAddress) {
        utils::output::info("Resuming job at address: " + utils::format_hex(jobAddress));
        Umodule::write<bool>(jobAddress + Offsets::TaskScheduler::JobBuisnessPlaying, false);
    }
    else {
        utils::output::warning("ResumeTask called with null job address.");
    }
}

bool RBX::TaskScheduler::RemoveTaskByName(const std::string& tarGetName) {
    utils::output::info("Removing job with name: " + tarGetName);
    uintptr_t scheduler = GetScheduler();
    uintptr_t jobListStart = Umodule::read<uintptr_t>(scheduler + Offsets::TaskScheduler::JobStart);

    for (uintptr_t currentJob = jobListStart; ; currentJob += Offsets::TaskScheduler::JobInterpation) {
        uintptr_t jobAddress = Umodule::read<uintptr_t>(currentJob);
        if (jobAddress == 0) {
            utils::output::info("End of job list reached.");
            break;
        }

        std::string jobName = RBX::Instance::ReadString(jobAddress + Offsets::TaskScheduler::JobName);
        if (jobName == tarGetName) {
            utils::output::info("Target job found: " + jobName + " — removing...");
            Umodule::write<uintptr_t>(currentJob, 0);
            return true;
        }
        else {
            utils::output::info("Scanned job: " + jobName);
        }
    }

    utils::output::error("Failed to find and remove job with name: " + tarGetName);
    return false;
}

void RBX::TaskScheduler::UpdateJobPriority(uintptr_t jobAddress, unsigned int newPriority) {
    if (jobAddress) {
        utils::output::info("Updating job priority at " + utils::format_hex(jobAddress) + " to " + std::to_string(newPriority));
        Umodule::write<unsigned int>(jobAddress + Offsets::TaskScheduler::JobPriority, newPriority);
    }
    else {
        utils::output::warning("UpdateJobPriority called with null job address.");
    }
}

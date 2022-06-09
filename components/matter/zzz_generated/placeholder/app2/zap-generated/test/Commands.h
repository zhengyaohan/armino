/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

// THIS FILE IS GENERATED BY ZAP

#pragma once

#include "TestCommand.h"

class Test_TC_DM_1_3_SimulatedSuite : public TestCommand
{
public:
    Test_TC_DM_1_3_SimulatedSuite() : TestCommand("Test_TC_DM_1_3_Simulated", 21)
    {
        AddArgument("nodeId", 0, UINT64_MAX, &mNodeId);
        AddArgument("cluster", &mCluster);
        AddArgument("endpoint", 0, UINT16_MAX, &mEndpoint);
        AddArgument("timeout", 0, UINT16_MAX, &mTimeout);
    }

    ~Test_TC_DM_1_3_SimulatedSuite() {}

private:
    chip::Optional<chip::NodeId> mNodeId;
    chip::Optional<chip::CharSpan> mCluster;
    chip::Optional<chip::EndpointId> mEndpoint;
    chip::Optional<uint16_t> mTimeout;

    chip::EndpointId GetEndpoint(chip::EndpointId endpoint) { return mEndpoint.HasValue() ? mEndpoint.Value() : endpoint; }

    //
    // Tests methods
    //

    void OnResponse(const chip::app::StatusIB & status, chip::TLV::TLVReader * data) override
    {
        bool shouldContinue = false;

        switch (mTestIndex - 1)
        {
        case 0:
            VerifyOrReturn(CheckValue("status", chip::to_underlying(status.mStatus), 0));
            shouldContinue = true;
            break;
        case 1:
            VerifyOrReturn(CheckValue("status", chip::to_underlying(status.mStatus), 0));
            shouldContinue = true;
            break;
        default:
            LogErrorOnFailure(ContinueOnChipMainThread(CHIP_ERROR_INVALID_ARGUMENT));
        }

        if (shouldContinue)
        {
            ContinueOnChipMainThread(CHIP_NO_ERROR);
        }
    }

    CHIP_ERROR DoTestStep(uint16_t testIndex) override
    {
        using namespace chip::app::Clusters;
        switch (testIndex)
        {
        case 0: {
            LogStep(0, "Wait for the device to be commissioned");
            SetIdentity(kIdentityAlpha);
            return WaitForCommissioning();
        }
        case 1: {
            LogStep(1, "Log OnOff Test Startup");
            SetIdentity(kIdentityAlpha);
            return Log("*** Basic Cluster Tests Ready");
        }
        case 2: {
            LogStep(2, "Query Data Model Revision");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::DataModelRevision::Id);
        }
        case 3: {
            LogStep(3, "Query Vendor Name");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::VendorName::Id);
        }
        case 4: {
            LogStep(4, "Query VendorID");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::VendorID::Id);
        }
        case 5: {
            LogStep(5, "Query Product Name");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::ProductName::Id);
        }
        case 6: {
            LogStep(6, "Query ProductID");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::ProductID::Id);
        }
        case 7: {
            LogStep(7, "Query Node Label");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::NodeLabel::Id);
        }
        case 8: {
            LogStep(8, "Query User Location");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::Location::Id);
        }
        case 9: {
            LogStep(9, "Query HardwareVersion");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::HardwareVersion::Id);
        }
        case 10: {
            LogStep(10, "Query HardwareVersionString");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::HardwareVersionString::Id);
        }
        case 11: {
            LogStep(11, "Query SoftwareVersion");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::SoftwareVersion::Id);
        }
        case 12: {
            LogStep(12, "Query SoftwareVersionString");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::SoftwareVersionString::Id);
        }
        case 13: {
            LogStep(13, "Query ManufacturingDate");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::ManufacturingDate::Id);
        }
        case 14: {
            LogStep(14, "Query PartNumber");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::PartNumber::Id);
        }
        case 15: {
            LogStep(15, "Query ProductURL");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::ProductURL::Id);
        }
        case 16: {
            LogStep(16, "Query ProductLabel");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::ProductLabel::Id);
        }
        case 17: {
            LogStep(17, "Query SerialNumber");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::SerialNumber::Id);
        }
        case 18: {
            LogStep(18, "Query LocalConfigDisabled");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::LocalConfigDisabled::Id);
        }
        case 19: {
            LogStep(19, "Query Reachable");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::Reachable::Id);
        }
        case 20: {
            LogStep(20, "Query UniqueID");
            return WaitAttribute(GetEndpoint(0), Basic::Id, Basic::Attributes::UniqueID::Id);
        }
        }
        return CHIP_NO_ERROR;
    }
};

class Test_TC_DM_3_3_SimulatedSuite : public TestCommand
{
public:
    Test_TC_DM_3_3_SimulatedSuite() : TestCommand("Test_TC_DM_3_3_Simulated", 6)
    {
        AddArgument("nodeId", 0, UINT64_MAX, &mNodeId);
        AddArgument("cluster", &mCluster);
        AddArgument("endpoint", 0, UINT16_MAX, &mEndpoint);
        AddArgument("timeout", 0, UINT16_MAX, &mTimeout);
    }

    ~Test_TC_DM_3_3_SimulatedSuite() {}

private:
    chip::Optional<chip::NodeId> mNodeId;
    chip::Optional<chip::CharSpan> mCluster;
    chip::Optional<chip::EndpointId> mEndpoint;
    chip::Optional<uint16_t> mTimeout;

    chip::EndpointId GetEndpoint(chip::EndpointId endpoint) { return mEndpoint.HasValue() ? mEndpoint.Value() : endpoint; }

    //
    // Tests methods
    //

    void OnResponse(const chip::app::StatusIB & status, chip::TLV::TLVReader * data) override
    {
        bool shouldContinue = false;

        switch (mTestIndex - 1)
        {
        case 0:
            VerifyOrReturn(CheckValue("status", chip::to_underlying(status.mStatus), 0));
            shouldContinue = true;
            break;
        default:
            LogErrorOnFailure(ContinueOnChipMainThread(CHIP_ERROR_INVALID_ARGUMENT));
        }

        if (shouldContinue)
        {
            ContinueOnChipMainThread(CHIP_NO_ERROR);
        }
    }

    CHIP_ERROR DoTestStep(uint16_t testIndex) override
    {
        using namespace chip::app::Clusters;
        switch (testIndex)
        {
        case 0: {
            LogStep(0, "Wait for the device to be commissioned");
            SetIdentity(kIdentityAlpha);
            return WaitForCommissioning();
        }
        case 1: {
            LogStep(1, "Wait for Scan Network Command");
            return WaitCommand(GetEndpoint(0), NetworkCommissioning::Id, NetworkCommissioning::Commands::ScanNetworks::Id);
        }
        case 2: {
            LogStep(2, "Wait for Add Wifi Network Command");
            VerifyOrdo(!ShouldSkip("WIFI"), return ContinueOnChipMainThread(CHIP_NO_ERROR));
            return WaitCommand(GetEndpoint(0), NetworkCommissioning::Id,
                               NetworkCommissioning::Commands::AddOrUpdateWiFiNetwork::Id);
        }
        case 3: {
            LogStep(3, "Wait for Update Thread Network Command");
            VerifyOrdo(!ShouldSkip("THREAD"), return ContinueOnChipMainThread(CHIP_NO_ERROR));
            return WaitCommand(GetEndpoint(0), NetworkCommissioning::Id,
                               NetworkCommissioning::Commands::AddOrUpdateThreadNetwork::Id);
        }
        case 4: {
            LogStep(4, "Wait for Enable Network Command");
            return WaitCommand(GetEndpoint(0), NetworkCommissioning::Id, NetworkCommissioning::Commands::ConnectNetwork::Id);
        }
        case 5: {
            LogStep(5, "Wait for Remove Network Command");
            VerifyOrdo(!ShouldSkip("WIFI | THREAD"), return ContinueOnChipMainThread(CHIP_NO_ERROR));
            return WaitCommand(GetEndpoint(0), NetworkCommissioning::Id, NetworkCommissioning::Commands::RemoveNetwork::Id);
        }
        }
        return CHIP_NO_ERROR;
    }
};

class Test_TC_DM_2_3_SimulatedSuite : public TestCommand
{
public:
    Test_TC_DM_2_3_SimulatedSuite() : TestCommand("Test_TC_DM_2_3_Simulated", 10)
    {
        AddArgument("nodeId", 0, UINT64_MAX, &mNodeId);
        AddArgument("cluster", &mCluster);
        AddArgument("endpoint", 0, UINT16_MAX, &mEndpoint);
        AddArgument("timeout", 0, UINT16_MAX, &mTimeout);
    }

    ~Test_TC_DM_2_3_SimulatedSuite() {}

private:
    chip::Optional<chip::NodeId> mNodeId;
    chip::Optional<chip::CharSpan> mCluster;
    chip::Optional<chip::EndpointId> mEndpoint;
    chip::Optional<uint16_t> mTimeout;

    chip::EndpointId GetEndpoint(chip::EndpointId endpoint) { return mEndpoint.HasValue() ? mEndpoint.Value() : endpoint; }

    //
    // Tests methods
    //

    void OnResponse(const chip::app::StatusIB & status, chip::TLV::TLVReader * data) override
    {
        bool shouldContinue = false;

        switch (mTestIndex - 1)
        {
        case 9:
            VerifyOrReturn(CheckValue("status", chip::to_underlying(status.mStatus), 0));
            shouldContinue = true;
            break;
        default:
            LogErrorOnFailure(ContinueOnChipMainThread(CHIP_ERROR_INVALID_ARGUMENT));
        }

        if (shouldContinue)
        {
            ContinueOnChipMainThread(CHIP_NO_ERROR);
        }
    }

    CHIP_ERROR DoTestStep(uint16_t testIndex) override
    {
        using namespace chip::app::Clusters;
        switch (testIndex)
        {
        case 0: {
            LogStep(0, "Wait for Arm Fail Safe");
            return WaitCommand(GetEndpoint(0), GeneralCommissioning::Id, GeneralCommissioning::Commands::ArmFailSafe::Id);
        }
        case 1: {
            LogStep(1, "Wait for Set Regulatory Config");
            return WaitCommand(GetEndpoint(0), GeneralCommissioning::Id, GeneralCommissioning::Commands::SetRegulatoryConfig::Id);
        }
        case 2: {
            LogStep(2, "Wait for Attestation Certificate Chain Request");
            return WaitCommand(GetEndpoint(0), OperationalCredentials::Id,
                               OperationalCredentials::Commands::CertificateChainRequest::Id);
        }
        case 3: {
            LogStep(3, "Wait for Attestation Certificate Chain Request");
            return WaitCommand(GetEndpoint(0), OperationalCredentials::Id,
                               OperationalCredentials::Commands::CertificateChainRequest::Id);
        }
        case 4: {
            LogStep(4, "Wait for Attestation Request");
            return WaitCommand(GetEndpoint(0), OperationalCredentials::Id,
                               OperationalCredentials::Commands::AttestationRequest::Id);
        }
        case 5: {
            LogStep(5, "Wait for CSR Request");
            return WaitCommand(GetEndpoint(0), OperationalCredentials::Id, OperationalCredentials::Commands::CSRRequest::Id);
        }
        case 6: {
            LogStep(6, "Wait for Add Trusted Root Certificate Request");
            return WaitCommand(GetEndpoint(0), OperationalCredentials::Id,
                               OperationalCredentials::Commands::AddTrustedRootCertificate::Id);
        }
        case 7: {
            LogStep(7, "Wait for Add Op NOC");
            return WaitCommand(GetEndpoint(0), OperationalCredentials::Id, OperationalCredentials::Commands::AddNOC::Id);
        }
        case 8: {
            LogStep(8, "Wait for Commissioning Complete");
            return WaitCommand(GetEndpoint(0), GeneralCommissioning::Id, GeneralCommissioning::Commands::CommissioningComplete::Id);
        }
        case 9: {
            LogStep(9, "Wait 3000ms");
            SetIdentity(kIdentityAlpha);
            return WaitForMs(3000);
        }
        }
        return CHIP_NO_ERROR;
    }
};

std::unique_ptr<TestCommand> GetTestCommand(std::string testName)
{
    if (testName == "Test_TC_DM_1_3_Simulated")
    {
        return std::unique_ptr<Test_TC_DM_1_3_SimulatedSuite>(new Test_TC_DM_1_3_SimulatedSuite());
    }
    if (testName == "Test_TC_DM_3_3_Simulated")
    {
        return std::unique_ptr<Test_TC_DM_3_3_SimulatedSuite>(new Test_TC_DM_3_3_SimulatedSuite());
    }
    if (testName == "Test_TC_DM_2_3_Simulated")
    {
        return std::unique_ptr<Test_TC_DM_2_3_SimulatedSuite>(new Test_TC_DM_2_3_SimulatedSuite());
    }

    return nullptr;
}

void PrintTestCommands()
{
    ChipLogError(chipTool, "Supported commands:");
    ChipLogError(chipTool, "\t* Test_TC_DM_1_3_Simulated");
    ChipLogError(chipTool, "\t* Test_TC_DM_3_3_Simulated");
    ChipLogError(chipTool, "\t* Test_TC_DM_2_3_Simulated");
}

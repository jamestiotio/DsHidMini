#include "Driver.h"
#include "Configuration.tmh"


void ConfigDeviceSpecificParse(
	_In_ const cJSON* DeviceNode,
	_Inout_ PDEVICE_CONTEXT Context
)
{
	cJSON* currentNode = NULL;

	currentNode = cJSON_GetObjectItem(DeviceNode, "HidDeviceMode");

	if (currentNode)
	{
		Context->Configuration.HidDeviceMode = (DS_HID_DEVICE_MODE)cJSON_GetNumberValue(currentNode);
	}
	else
	{
		Context->Configuration.HidDeviceMode = DsHidMiniDeviceModeXInputHIDCompatible;
	}

	currentNode = cJSON_GetObjectItem(DeviceNode, "MuteDigitalPressureButtons");

	if (currentNode)
	{
		Context->Configuration.MuteDigitalPressureButtons = cJSON_GetNumberValue(currentNode) > 0.0f;
	}

	currentNode = cJSON_GetObjectItem(DeviceNode, "DisableAutoPairing");

	if (currentNode)
	{
		Context->Configuration.DisableAutoPairing = cJSON_GetNumberValue(currentNode) > 0.0f;
	}

	currentNode = cJSON_GetObjectItem(DeviceNode, "IsOutputRateControlEnabled");

	if (currentNode)
	{
		Context->Configuration.IsOutputRateControlEnabled = cJSON_GetNumberValue(currentNode) > 0.0f;
	}

	currentNode = cJSON_GetObjectItem(DeviceNode, "OutputRateControlPeriodMs");

	if (currentNode)
	{
		Context->Configuration.OutputRateControlPeriodMs = (UCHAR)cJSON_GetNumberValue(currentNode);
	}
	else
	{
		Context->Configuration.OutputRateControlPeriodMs = 150;
	}

	currentNode = cJSON_GetObjectItem(DeviceNode, "IsOutputDeduplicatorEnabled");

	if (currentNode)
	{
		Context->Configuration.IsOutputDeduplicatorEnabled = cJSON_GetNumberValue(currentNode) > 0.0f;
	}

	currentNode = cJSON_GetObjectItem(DeviceNode, "WirelessIdleTimeoutPeriodMs");

	if (currentNode)
	{
		Context->Configuration.WirelessIdleTimeoutPeriodMs = (ULONG)cJSON_GetNumberValue(currentNode);
	}
	else
	{
		Context->Configuration.WirelessIdleTimeoutPeriodMs = 300000;
	}
}

_Must_inspect_result_
NTSTATUS
ConfigLoadForDevice(
	_Inout_ PDEVICE_CONTEXT Context
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	CHAR programDataPath[MAX_PATH];
	CHAR configFilePath[MAX_PATH];
	CHAR deviceAddress[13];
	FILE* fp = NULL;
	PCHAR content = NULL;
	cJSON* config_json = NULL;

	FuncEntry(TRACE_CONFIG);

	do
	{
		if (GetEnvironmentVariableA(
			CONFIG_ENV_VAR_NAME,
			programDataPath,
			MAX_PATH
		) == 0)
		{
			status = STATUS_NOT_FOUND;
			break;
		}

		if (sprintf_s(
			configFilePath,
			MAX_PATH / sizeof(WCHAR),
			"%s\\%s\\%s",
			programDataPath,
			CONFIG_SUB_DIR_NAME,
			CONFIG_FILE_NAME
		) == -1)
		{
			status = STATUS_BUFFER_OVERFLOW;
			break;
		}

		errno_t error;

		if ((error = fopen_s(&fp, configFilePath, "r")) != 0)
		{
			TraceVerbose(
				TRACE_CONFIG,
				"Configuration file %s not accessible (errno: %d)",
				configFilePath,
				error
			);

			status = STATUS_ACCESS_DENIED;
			break;
		}

		fseek(fp, 0L, SEEK_END);
		const long numBytes = ftell(fp);
		fseek(fp, 0L, SEEK_SET);

		content = (char*)calloc(numBytes, sizeof(char));

		if (content == NULL)
		{
			status = STATUS_NO_MEMORY;
			break;
		}

		fread(content, sizeof(char), numBytes, fp);

		config_json = cJSON_Parse(content);

		if (config_json == NULL)
		{
			const char* error_ptr = cJSON_GetErrorPtr();
			if (error_ptr != NULL)
			{
				TraceError(
					TRACE_CONFIG,
					"JSON error: %s",
					error_ptr
				);
			}

			status = STATUS_NOT_FOUND;
			break;
		}

		switch (Context->ConnectionType)
		{
		case DsDeviceConnectionTypeUsb:

			sprintf_s(
				deviceAddress,
				ARRAYSIZE(deviceAddress),
				"%02X%02X%02X%02X%02X%02X",
				Context->DeviceAddress.Address[0],
				Context->DeviceAddress.Address[1],
				Context->DeviceAddress.Address[2],
				Context->DeviceAddress.Address[3],
				Context->DeviceAddress.Address[4],
				Context->DeviceAddress.Address[5]
			);

			break;
		case DsDeviceConnectionTypeBth:

			sprintf_s(
				deviceAddress,
				ARRAYSIZE(deviceAddress),
				"%012llX",
				*(PULONGLONG)&Context->DeviceAddress
			);

			break;
		default:
			status = STATUS_INVALID_PARAMETER;
			break;
		}

		const cJSON* devicesNode = cJSON_GetObjectItem(config_json, "Devices");

		cJSON* deviceNode = cJSON_GetObjectItem(devicesNode, deviceAddress);

		if (deviceNode == NULL)
		{
			break;
		}

		status = STATUS_SUCCESS;

		TraceVerbose(
			TRACE_CONFIG,
			"Found device config for %s",
			deviceAddress
		);

		ConfigDeviceSpecificParse(deviceNode, Context);

	} while (FALSE);

	//
	// Set defaults here
	// 
	if (!NT_SUCCESS(status))
	{
		Context->Configuration.HidDeviceMode = DsHidMiniDeviceModeXInputHIDCompatible;
		Context->Configuration.IsOutputDeduplicatorEnabled = TRUE;
		Context->Configuration.OutputRateControlPeriodMs = 150;
		Context->Configuration.WirelessIdleTimeoutPeriodMs = 300000;
	}

	if (config_json)
	{
		cJSON_Delete(config_json);
	}

	if (content)
	{
		free(content);
	}

	if (fp)
	{
		fclose(fp);
	}

	FuncExit(TRACE_CONFIG, "status=%!STATUS!", status);

	return status;
}

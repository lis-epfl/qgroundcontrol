/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#include "MAVRICAutoPilotPlugin.h"
//#include "AirframeComponent.h"
//#include "RadioComponent.h"
//#include "SensorsComponent.h"
//#include "FlightModesComponent.h"
#include "AutoPilotPluginManager.h"
#include "UASManager.h"
#include "QGCUASParamManagerInterface.h"

/// @file
///     @brief This is the AutoPilotPlugin implementatin for the MAVRIC type.
///     @author Nicolas Dousse <nic.dousse@gmail.com>

enum MAVRIC_MAV_MODE_CUSTOM
{
	MAVRIC_CUSTOM_BASE_MODE = 0,
	MAVRIC_CUSTOM_CLIMB_TO_SAFE_ALT = 1,				///< First critical behavior
	MAVRIC_CUSTOM_FLY_TO_HOME_WP = 2,					///< Second critical behavior, comes after CLIMB_TO_SAFE_ALT
	MAVRIC_CUSTOM_CRITICAL_LAND = 4,					///< Third critical behavior, comes after FLY_TO_HOME_WP

	MAVRIC_CUSTOM_DESCENT_TO_SMALL_ALTITUDE = 8,		///< First auto landing behavior
	MAVRIC_CUSTOM_DESCENT_TO_GND = 16,					///< Second auto landing behavior, comes after DESCENT_TO_SMAL_ALTITUDE
	
	MAVRIC_CUSTOM_COLLISION_AVOIDANCE = 32				///< Collision avoidance
};

MAVRICAutoPilotPlugin::MAVRICAutoPilotPlugin(UASInterface* uas, QObject* parent) :
    AutoPilotPlugin(parent),
    _uas(uas)
{
    Q_UNUSED(uas);


    _parameterFacts = new GenericParameterFacts(uas, this);
    Q_CHECK_PTR(_parameterFacts);
    
    connect(_parameterFacts, &GenericParameterFacts::factsReady, this, &MAVRICAutoPilotPlugin::pluginReady);


}

MAVRICAutoPilotPlugin::~MAVRICAutoPilotPlugin()
{
}

QList<AutoPilotPluginManager::FullMode_t> MAVRICAutoPilotPlugin::getModes(void)
{
    AutoPilotPluginManager::FullMode_t          fullMode;
    QList<AutoPilotPluginManager::FullMode_t>   modeList;
    
    // Attitude command mode
    fullMode.baseMode = MAV_MODE_FLAG_MANUAL_INPUT_ENABLED;
    fullMode.customMode = 0;
    modeList << fullMode;

	// Velocity command mode
    fullMode.baseMode = MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_STABILIZE_ENABLED;
    fullMode.customMode = 0;
    modeList << fullMode;

    // Position hold command mode
    fullMode.baseMode = MAV_MODE_FLAG_MANUAL_INPUT_ENABLED | MAV_MODE_FLAG_STABILIZE_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED;
    fullMode.customMode = 0;
    modeList << fullMode;

    // GPS navigation command mode
    fullMode.baseMode = MAV_MODE_FLAG_STABILIZE_ENABLED | MAV_MODE_FLAG_GUIDED_ENABLED | MAV_MODE_FLAG_AUTO_ENABLED;
    fullMode.customMode = 0;
    modeList << fullMode;

    return modeList;
}

QString MAVRICAutoPilotPlugin::getShortModeText(uint8_t baseMode, uint32_t customMode)
{
    QString mode;
    
    //Q_ASSERT(baseMode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED);

    // use base_mode - not autopilot-specific
    if (baseMode == 0) {
        mode = "|PREFLIGHT";
    } else if (baseMode & MAV_MODE_FLAG_DECODE_POSITION_AUTO) {
        mode = "|GPS";
    } else if (baseMode & MAV_MODE_FLAG_DECODE_POSITION_GUIDED) {
        mode = "|POS_HOLD";
    } else if (baseMode & MAV_MODE_FLAG_DECODE_POSITION_STABILIZE) {
        mode = "|VELOCITY";
    } else if (baseMode & MAV_MODE_FLAG_DECODE_POSITION_MANUAL) {
        mode = "|ATTITUDE";
    }


	//if (baseMode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED) {
		if (customMode & MAVRIC_CUSTOM_CLIMB_TO_SAFE_ALT)
		{
			mode += "|CLIMB_TO_SAFE_ALT";
		} else if(customMode & MAVRIC_CUSTOM_FLY_TO_HOME_WP)
		{
			mode += "|FLY_TO_HOME";
		} else if(customMode & MAVRIC_CUSTOM_CRITICAL_LAND)
		{
			mode += "|CRITICAL_LANDING";
		} else if(customMode & MAVRIC_CUSTOM_DESCENT_TO_SMALL_ALTITUDE)
		{
			mode += "|DESCENDING";
		} else if(customMode & MAVRIC_CUSTOM_DESCENT_TO_GND)
		{
			mode += "|LANDING";
		}else if(customMode & MAVRIC_CUSTOM_COLLISION_AVOIDANCE)
		{
			mode += "|COLL_AVOID";
		}
    //}
    
    return mode;
}

void MAVRICAutoPilotPlugin::clearStaticData(void)
{
    // No Static data yet
}

const QVariantList& MAVRICAutoPilotPlugin::components(void)
{
    static QVariantList emptyList;
    
    return emptyList;
}

const QVariantMap& MAVRICAutoPilotPlugin::parameters(void)
{
    return _parameterFacts->factMap();
}

QUrl MAVRICAutoPilotPlugin::setupBackgroundImage(void)
{
    return QUrl::fromUserInput("qrc:/qml/px4fmu_2.x.png");
}
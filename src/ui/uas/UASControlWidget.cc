/*=====================================================================

PIXHAWK Micro Air Vehicle Flying Robotics Toolkit

(c) 2009, 2010 PIXHAWK PROJECT  <http://pixhawk.ethz.ch>

This file is part of the PIXHAWK project

    PIXHAWK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    PIXHAWK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with PIXHAWK. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Definition of widget controlling one MAV
 *
 *   @author Lorenz Meier <mavteam@student.ethz.ch>
 *
 */

#include <QString>
#include <QTimer>
#include <QLabel>
#include <QProcess>
#include <QPalette>

#include "UASControlWidget.h"
#include <UASManager.h>
#include <UAS.h>
#include "QGC.h"
#include "AutoPilotPluginManager.h"
#include "MAVLinkProtocol.h"

UASControlWidget::UASControlWidget(QWidget *parent) : QWidget(parent),
    uasID(-1),
    modeIdx(0),
    armed(false)
{
    ui.setupUi(this);

    this->setUAS(UASManager::instance()->getActiveUAS());

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setUAS(UASInterface*)));
    connect(ui.modeComboBox, SIGNAL(activated(int)), this, SLOT(setMode(int)));
    connect(ui.setModeButton, SIGNAL(clicked()), this, SLOT(transmitMode()));

    ui.gridLayout->setAlignment(Qt::AlignTop);

    mavlink = MAVLinkProtocol::instance();
}

void UASControlWidget::updateModesList()
{
    if (this->uasID == 0) {
        return;
    }
    
    UASInterface*uas = UASManager::instance()->getUASForId(this->uasID);
    Q_ASSERT(uas);
    
    _modeList = AutoPilotPluginManager::instance()->getModes(uas->getAutopilotType());

    // Set combobox items
    ui.modeComboBox->clear();
    foreach (AutoPilotPluginManager::FullMode_t fullMode, _modeList) {
        ui.modeComboBox->addItem(uas->getShortModeTextFor(fullMode.baseMode, fullMode.customMode).remove(0, 2));
    }

    // Select first mode in list
    modeIdx = 0;
    ui.modeComboBox->setCurrentIndex(modeIdx);
    ui.modeComboBox->update();
}

void UASControlWidget::setUAS(UASInterface* uas)
{
    if (this->uasID > 0) {
        UASInterface* oldUAS = UASManager::instance()->getUASForId(this->uasID);
        if (oldUAS) {
            disconnect(ui.controlButton, SIGNAL(clicked()), oldUAS, SLOT(armSystem()));
            disconnect(ui.liftoffButton, SIGNAL(clicked()), oldUAS, SLOT(launch()));
            disconnect(ui.landButton, SIGNAL(clicked()), oldUAS, SLOT(land()));
            disconnect(ui.shutdownButton, SIGNAL(clicked()), oldUAS, SLOT(shutdown()));
            //connect(ui.setHomeButton, SIGNAL(clicked()), uas, SLOT(setLocalOriginAtCurrentGPSPosition()));
            disconnect(oldUAS, SIGNAL(modeChanged(int,QString,QString)), this, SLOT(updateMode(int, QString, QString)));
            disconnect(oldUAS, SIGNAL(statusChanged(int)), this, SLOT(updateState(int)));
            
            disconnect(ui.nextButton,SIGNAL(clicked()),this,SLOT(next_clicked()));
            disconnect(ui.setNewHomeButton,SIGNAL(clicked()),this,SLOT(newHome_clicked()));
            disconnect(ui.startButton,SIGNAL(clicked()),this,SLOT(start_clicked()));
            disconnect(ui.stopButton,SIGNAL(clicked()),this,SLOT(stop_clicked()));
            disconnect(ui.isArrivedButton,SIGNAL(clicked()),this,SLOT(isArrived_clicked()));
        }
    }

    // Connect user interface controls
    if (uas) {
        uasActive = uas;
        connect(ui.controlButton, SIGNAL(clicked()), this, SLOT(cycleContextButton()));
        connect(ui.liftoffButton, SIGNAL(clicked()), uas, SLOT(launch()));
        connect(ui.landButton, SIGNAL(clicked()), uas, SLOT(land()));
        connect(ui.shutdownButton, SIGNAL(clicked()), uas, SLOT(shutdown()));
        //connect(ui.setHomeButton, SIGNAL(clicked()), uas, SLOT(setLocalOriginAtCurrentGPSPosition()));
        connect(uas, SIGNAL(modeChanged(int, QString, QString)), this, SLOT(updateMode(int, QString, QString)));
        connect(uas, SIGNAL(statusChanged(int)), this, SLOT(updateState(int)));

        connect(ui.nextButton,SIGNAL(clicked()),this,SLOT(next_clicked()));
        connect(ui.setNewHomeButton,SIGNAL(clicked()),this,SLOT(newHome_clicked()));
        connect(ui.startButton,SIGNAL(clicked()),this,SLOT(start_clicked()));
        connect(ui.stopButton,SIGNAL(clicked()),this,SLOT(stop_clicked()));
        connect(ui.isArrivedButton,SIGNAL(clicked()),this,SLOT(isArrived_clicked()));

        ui.controlStatusLabel->setText(tr("Connected to ") + uas->getUASName());

        this->uasID = uas->getUASID();
        setBackgroundColor(uas->getColor());

        this->updateModesList();
        this->updateArmText();

    } else {
        this->uasID = -1;
    }
}

UASControlWidget::~UASControlWidget()
{

}

void UASControlWidget::updateArmText()
{
    if (armed) {
        ui.controlButton->setText(tr("DISARM SYSTEM"));
    } else {
        ui.controlButton->setText(tr("ARM SYSTEM"));
    }
}

/**
 * Set the background color based on the MAV color. If the MAV is selected as the
 * currently actively controlled system, the frame color is highlighted
 */
void UASControlWidget::setBackgroundColor(QColor color)
{
    // UAS color
    QColor uasColor = color;
    QString colorstyle;
    QString borderColor = "#4A4A4F";
    borderColor = "#FA4A4F";
    uasColor = uasColor.darker(400);
    colorstyle = colorstyle.sprintf("QLabel { border-radius: 3px; padding: 0px; margin: 0px; background-color: #%02X%02X%02X; border: 0px solid %s; }",
                                    uasColor.red(), uasColor.green(), uasColor.blue(), borderColor.toStdString().c_str());
    setStyleSheet(colorstyle);
    QPalette palette = this->palette();
    palette.setBrush(QPalette::Window, QBrush(uasColor));
    setPalette(palette);
    setAutoFillBackground(true);
}


void UASControlWidget::updateMode(int uas, QString mode, QString description)
{
    Q_UNUSED(uas);
    Q_UNUSED(mode);
    Q_UNUSED(description);
}

void UASControlWidget::updateState(int state)
{
    switch (state) {
    case (int)MAV_STATE_ACTIVE:
        armed = true;
        break;
    case (int)MAV_STATE_STANDBY:
        armed = false;
        break;
    }
    this->updateArmText();
}

/**
 * Called by the button
 */
void UASControlWidget::setMode(int mode)
{
    // Adapt context button mode
    modeIdx = mode;
    ui.modeComboBox->blockSignals(true);
    ui.modeComboBox->setCurrentIndex(mode);
    ui.modeComboBox->blockSignals(false);

    emit changedMode(mode);
}

void UASControlWidget::transmitMode()
{
    UASInterface* uas_iface = UASManager::instance()->getUASForId(this->uasID);
    if (uas_iface) {
        if (modeIdx >= 0 && modeIdx < _modeList.count()) {
            AutoPilotPluginManager::FullMode_t fullMode = _modeList[modeIdx];
            // include armed state
            if (armed) {
                fullMode.baseMode |= MAV_MODE_FLAG_SAFETY_ARMED;
            } else {
                fullMode.baseMode &= ~MAV_MODE_FLAG_SAFETY_ARMED;
            }

            UAS* uas = dynamic_cast<UAS*>(uas_iface);

            if (uas->isHilEnabled() || uas->isHilActive()) {
                fullMode.baseMode |= MAV_MODE_FLAG_HIL_ENABLED;
            } else {
                fullMode.baseMode &= ~MAV_MODE_FLAG_HIL_ENABLED;
            }

            uas->setMode(fullMode.baseMode, fullMode.customMode);
            QString modeText = ui.modeComboBox->currentText();

            ui.lastActionLabel->setText(QString("Sent new mode %1 to %2").arg(modeText).arg(uas->getUASName()));
        }
    }
}

void UASControlWidget::cycleContextButton()
{
    UAS* uas = dynamic_cast<UAS*>(UASManager::instance()->getUASForId(this->uasID));
    if (uas) {
        if (!armed) {
            uas->armSystem();
            ui.lastActionLabel->setText(QString("Arm %1").arg(uas->getUASName()));
        } else {
            uas->disarmSystem();
            ui.lastActionLabel->setText(QString("Disarm %1").arg(uas->getUASName()));
        }
    }
}

void UASControlWidget::next_clicked()
{
    if(mavlink)
    {
        mavlink_message_t msg;

        mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, uasID, MAV_COMP_ID_MISSIONPLANNER, MAV_CMD_MISSION_START, 1, 1, 1, 1, 0, 0, 0, 0);
        mavlink->sendMessage(msg);

    }
}

void UASControlWidget::newHome_clicked()
{
    if(uasActive)
    {
        mavlink_message_t msg;

        double lat = UASManager::instance()->getHomeLatitude();
        double lon = UASManager::instance()->getHomeLongitude();
        double alt = UASManager::instance()->getHomeAltitude();

        mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, uasID, 0, MAV_CMD_DO_SET_HOME, 1, 0, 0, 0, 0, lat, lon, alt);
        
        mavlink->sendMessage(msg);
    }
}

void UASControlWidget::start_clicked()
{
    if(uasActive)
    {
        mavlink_message_t msg;
        mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, uasID, 0, MAV_CMD_OVERRIDE_GOTO, 1, MAV_GOTO_DO_CONTINUE, 0, 0, 0, 0, 0, 0);
        uasActive->sendMessage(msg);
    }
    
}

void UASControlWidget::stop_clicked()
{
    if(uasActive)
    {
        mavlink_message_t msg;
        mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, uasID, 0, MAV_CMD_OVERRIDE_GOTO, 1, MAV_GOTO_DO_HOLD, MAV_GOTO_HOLD_AT_CURRENT_POSITION, 0, 0, 0, 0, 0);
        uasActive->sendMessage(msg);
    }
}

void UASControlWidget::isArrived_clicked()
{
    if(uasActive)
    {
        mavlink_message_t msg;
        mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, uasID, MAV_COMP_ID_MISSIONPLANNER, MAV_CMD_CONDITION_DISTANCE, 1, 0, 32, 0, 0, 0, 0, 0);
        uasActive->sendMessage(msg);
    }
}
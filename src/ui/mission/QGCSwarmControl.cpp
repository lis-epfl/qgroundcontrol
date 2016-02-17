/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

// The above copyright block should be at the top of every file.

/// @file
///     @brief This file is used to control a group of muliple robot 
///             having in mind the MAV'RIC framework. 
///
///     @author Nicolas Dousse <nic.dousse@gmail.com>

#include "QGCSwarmControl.h"
#include "ui_QGCSwarmControl.h"

#include <QMessageBox>
#include "UAS.h"
#include "QGCMAVLink.h"
#include "LinkManager.h"
#include <QRadioButton>
#include <QString>
#include <QListWidget>
#include "UASManager.h"

const unsigned int QGCSwarmControl::updateInterval = 5000U;

QGCSwarmControl::QGCSwarmControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QGCSwarmControl)
{
    ui->setupUi(this);

    connect(ui->continueAllButton,SIGNAL(clicked()),this,SLOT(continueAllButton_clicked()));
    connect(ui->Return2Start,SIGNAL(clicked()),this,SLOT(Return2startButton_clicked()));
    connect(ui->scenarioLaunch,SIGNAL(clicked()),this,SLOT(launchScenario_clicked()));
    connect(ui->autoLanding,SIGNAL(clicked()),this,SLOT(autoLanding_clicked()));
    connect(ui->autoTakeoff,SIGNAL(clicked()),this,SLOT(autoTakeoff_clicked()));

    connect(ui->startLogging,SIGNAL(clicked()),this,SLOT(startLogging_clicked()));
    connect(ui->stopLogging,SIGNAL(clicked()),this,SLOT(stopLogging_clicked()));

    connect(ui->startButton,SIGNAL(clicked()),this,SLOT(startButton_clicked()));
    connect(ui->stopButton,SIGNAL(clicked()),this,SLOT(stopButton_clicked()));

    connect(ui->armButton,SIGNAL(clicked()),this,SLOT(armButton_clicked()));
    connect(ui->disarmButton,SIGNAL(clicked()),this,SLOT(disarmButton_clicked()));

    connect(ui->selectButton,SIGNAL(clicked()),this,SLOT(selectButton_clicked()));

    connect(ui->setHomeButton,SIGNAL(clicked()),this,SLOT(sendNewHomePosition()));

    mavlink = MAVLinkProtocol::instance();

    uas =  UASManager::instance()->getActiveUAS();
    uas_previous = UASManager::instance()->getActiveUAS();

    UASlist = UASManager::instance()->getUASList();

    mode_init = false;

    QListWidgetItem* item;
    foreach(UASInterface* uasNew, UASlist)
    {
        item = uasToItemMapping[uasNew];
        if (!item)
        {
            UASCreated(uasNew);
        }
    }

    connect(UASManager::instance(),SIGNAL(UASCreated(UASInterface*)),this,SLOT(UASCreated(UASInterface*)));
    connect(UASManager::instance(),SIGNAL(UASDeleted(UASInterface*)),this,SLOT(RemoveUAS(UASInterface*)));

    connect(ui->listWidget,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(ListWidgetClicked(QListWidgetItem*)));
    connect(UASManager::instance(),SIGNAL(activeUASSet(UASInterface*)),this,SLOT(newActiveUAS(UASInterface*)));

    connect(ui->remoteList,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(remoteItem_clicked(QListWidgetItem*)));

    connect(this,SIGNAL(uasTextReceived(UASInterface*, QString)),this,SLOT(textMessageReceived(UASInterface*, QString)));

    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(refreshView()));
    updateTimer.start(updateInterval);

    connect(ui->setParameters,SIGNAL(clicked()),this,SLOT(setParameters_clicked()));

    connect(ui->remoteButton,SIGNAL(clicked()),this,SLOT(remoteButton_clicked()));

    connect(ui->modeComboBox, SIGNAL(activated(int)), this, SLOT(setMode(int)));

    connect(ui->strategyLaunch, SIGNAL(clicked()),this,SLOT(strategyLaunchClicked()));

    all_selected = false;

    ui->disarmButton->setAutoFillBackground(true);
    ui->disarmButton->setStyleSheet("background-color: rgb(255, 0, 0); color: rgb(0, 0, 0)");

    wptReachedCnt = 0;
}

QGCSwarmControl::~QGCSwarmControl()
{
    delete ui;
}

void QGCSwarmControl::continueAllButton_clicked()
{
    qDebug() << "continueAllButton clicked";

    wptReachedCnt = 0;
    ui->wptReachedValue->setText(QString::number(wptReachedCnt));
    
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, 0, MAV_COMP_ID_MISSIONPLANNER, MAV_CMD_MISSION_START, 1, 1, 1, 0, 0, 0, 0, 0);
    mavlink->sendMessage(msg);
}

void QGCSwarmControl::Return2startButton_clicked()
{
    qDebug() << "Return2startButton clicked";

    mavlink_message_t msg;
    mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, 0, MAV_COMP_ID_MISSIONPLANNER, MAV_CMD_NAV_RETURN_TO_LAUNCH, 1, 0, 0, 0, 0, 0, 0, 0);
    mavlink->sendMessage(msg);
}

void QGCSwarmControl::launchScenario_clicked()
{
    qDebug() << "launchScenario clicked";

    int scenarioNum;

    scenarioNum = ui->comboBoxScenario->currentIndex() + 1;

    qDebug() << "scenarioNum:" + QString::number(scenarioNum);

    int autoContinue = 0;
    if(ui->autoContinueCheckBox->checkState() == Qt::Checked)
    {
        autoContinue = 1;
    }

    mavlink_message_t msg;
    mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, 0, MAV_COMP_ID_MISSIONPLANNER, MAV_CMD_CONDITION_LAST, 1, scenarioNum, ui->radiusSpinBox->value(), ui->numVhcSpinBox->value(),  ui->altitudeSpinBox->value(), autoContinue, 0, 0);
    mavlink->sendMessage(msg);
}

void QGCSwarmControl::autoLanding_clicked()
{
    qDebug() << "autoLanding clicked";

    mavlink_message_t msg;
    mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, 0, 0, MAV_CMD_NAV_LAND, 1, 0, 0, 0, 0, 0, 0, 0);
    mavlink->sendMessage(msg);
}

void QGCSwarmControl::autoTakeoff_clicked()
{
    qDebug() << "autoTakeoff clicked";

    mavlink_message_t msg;
    mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, 0, 0, MAV_CMD_NAV_TAKEOFF, 1, 0, 0, 0, 0, 0, 0, 0);
    mavlink->sendMessage(msg);
}

void QGCSwarmControl::startLogging_clicked()
{
    qDebug() << "startLogging clicked";

    mavlink_message_t msg;
    mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, 0, 0, MAV_CMD_DO_SET_PARAMETER, 1, 1, 0, 0, 0, 0, 0, 0);
    mavlink->sendMessage(msg);
}

void QGCSwarmControl::stopLogging_clicked()
{
    qDebug() << "stopLogging clicked";

    mavlink_message_t msg;
    mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, 0, 0, MAV_CMD_DO_SET_PARAMETER, 1, 0, 0, 0, 0, 0, 0, 0);
    mavlink->sendMessage(msg);
}

void QGCSwarmControl::UASCreated(UASInterface* uas)
{
    Q_ASSERT(uas);

    if (uas)
    {
        QString idstring;
        QStringList idstringList;
        if (uas->getUASName() == "")
        {
            idstring = tr("UAS ") + QString::number(uas->getUASID());
            idstringList << tr("UAS ") + QString::number(uas->getUASID());
        }
        else
        {
            idstring = uas->getUASName();
            idstringList << uas->getUASName();
        }

        QListWidgetItem* item = new QListWidgetItem(idstring);

        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);

        item->setForeground(uas->getColor());

        int id = ((uas->getUASID()-1)%10)+1;
        itemToID[item] = id;

        int i = 0;
        bool inserted = false;
        if (ui->listWidget->count() == 0)
        {
            ui->listWidget->addItem(item);
        }
        else
        {
            QListWidgetItem* itemSwap;
            while (i < ui->listWidget->count() && !inserted)
            {
                itemSwap = ui->listWidget->item(i);
                if (itemToID[itemSwap] > itemToID[item])
                {
                    ui->listWidget->insertItem(i,item);
                    inserted = true;
                }
                else
                {
                    i++;
                }
            }
            if (!inserted)
            {
                ui->listWidget->addItem(item);
            }
        }

        uasToItemMapping[uas] = item;
        itemToUasMapping[item] = uas;

        connect(uas,SIGNAL(textMessageReceived(int,int,int,QString)),this,SLOT(textEmit(int,int,int,QString)));

        QListWidgetItem* itemRemote = new QListWidgetItem(idstring);
        itemRemote->setFlags(itemRemote->flags() | Qt::ItemIsUserCheckable);
        itemRemote->setCheckState(Qt::Checked);

        itemRemote->setForeground(uas->getColor());

        uasToItemRemote[uas] = itemRemote;
        itemToUasRemote[itemRemote] = uas;

        itemToIDRemote[itemRemote] = id;

        
        if (ui->remoteList->count() == 0)
        {
            ui->remoteList->addItem(itemRemote);
        }
        else
        {
            i = 0;
            inserted = false;
            QListWidgetItem* itemSwapRemote;
            while (i < ui->remoteList->count() && !inserted)
            {
                itemSwapRemote = ui->remoteList->item(i);
                if (itemToIDRemote[itemSwapRemote] > itemToIDRemote[itemRemote])
                {
                    ui->remoteList->insertItem(i,itemRemote);
                    inserted = true;
                }
                else
                {
                    i++;
                }
            }
            if (!inserted)
            {
                ui->remoteList->addItem(itemRemote);
            }
        }

        if(!mode_init)
        {
            updateModesList(uas);
            mode_init = true;
        }
        
    }
}

void QGCSwarmControl::updateModesList(UASInterface* uas)
{
    if (!uas)
    {
        return;
    }
    
    _modeList = AutoPilotPluginManager::instance()->getModes(uas->getAutopilotType());

    // Set combobox items
    ui->modeComboBox->clear();
    foreach (AutoPilotPluginManager::FullMode_t fullMode, _modeList)
    {
        ui->modeComboBox->addItem(uas->getShortModeTextFor(fullMode.baseMode, fullMode.customMode).remove(0, 2));
    }

    // Select first mode in list
    modeIdx = 0;
    ui->modeComboBox->setCurrentIndex(modeIdx);
    ui->modeComboBox->update();
}

void QGCSwarmControl::RemoveUAS(UASInterface* uas)
{
    qDebug() << "Removing UAS from SwarmControl";

    QListWidgetItem* item = uasToItemMapping[uas];
    uasToItemMapping.remove(uas);

    itemToUasMapping.remove(item);

    //ui->listWidget->removeItemWidget(item);
    ui->listWidget->takeItem(ui->listWidget->row(item));
    delete item;

    item = uasToItemRemote[uas];
    uasToItemRemote.remove(uas);

    itemToUasRemote.remove(item);

    ui->remoteList->takeItem(ui->remoteList->row(item));
    delete item;

    UASlist = UASManager::instance()->getUASList();
}

void QGCSwarmControl::ListWidgetClicked(QListWidgetItem* item)
{

    QMap<QListWidgetItem*,UASInterface*>::const_iterator it = itemToUasMapping.constFind(item);

    UASInterface* uas = it.value();

    if (uas != UASManager::instance()->getActiveUAS())
    {
        UASManager::instance()->setActiveUAS(uas);
    }
}

void QGCSwarmControl::newActiveUAS(UASInterface* uas)
{
    QListWidgetItem* item;

    if (uas_previous)
    {
        item = uasToItemMapping[uas_previous];
        item->setCheckState(Qt::Unchecked);
    }
    uas_previous = uas;


    item = uasToItemMapping[uas];
    ui->listWidget->setItemSelected(item,true);

    item->setCheckState(Qt::Checked);

}

void QGCSwarmControl::textEmit(int uasid, int component, int severity, QString message)
{
    Q_UNUSED(uasid);
    Q_UNUSED(component);
    Q_UNUSED(severity);

    UASInterface* uas = (UASInterface*)sender();

    emit uasTextReceived(uas,message);
}

void QGCSwarmControl::textMessageReceived(UASInterface* uas, QString message)
{
    QListWidgetItem* item;

    if (uas)
    {
        qDebug() << "UAS id:" << QString::number(uas->getUASID());
        item = uasToItemMapping[uas];

        qDebug() << message;

        if (message.contains("reached waypoint"))
        {
            wptReachedCnt++;
            qDebug() << QString::number(wptReachedCnt);
            ui->wptReachedValue->setText(QString::number(wptReachedCnt));
        } else if (message.contains("SUCCESS"))
        {
            item->setBackground(Qt::green);
        }
        else
        {
            item->setBackground(Qt::red);
        }
    }
}

void QGCSwarmControl::refreshView()
{
    QListWidgetItem* item;

    foreach(UASInterface* uas, UASManager::instance()->getUASList())
    {
        item = uasToItemMapping[uas];
        item->setBackground(Qt::transparent);
    }
}
void QGCSwarmControl::setParameters_clicked()
{
    float param1 = ui->spinBoxParam1->value();
    float param2 = ui->spinBoxParam2->value();
    float param3 = ui->spinBoxParam3->value();
    float param4 = ui->spinBoxParam4->value();
    float param5 = ui->spinBoxParam5->value();
    float param6 = ui->spinBoxParam6->value();
    float param7 = ui->spinBoxParam7->value();

    mavlink_message_t msg;
    mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, 0, 0, MAV_CMD_NAV_PATHPLANNING, 1, param1, param2, param3, param4, param5, param6, param7);
    mavlink->sendMessage(msg);
}

void QGCSwarmControl::startButton_clicked()
{
    //MAV_CMD_OVERRIDE_GOTO=252, /* Hold / continue the current action |MAV_GOTO_DO_HOLD: hold MAV_GOTO_DO_CONTINUE: continue with next item in mission plan| MAV_GOTO_HOLD_AT_CURRENT_POSITION: Hold at current position MAV_GOTO_HOLD_AT_SPECIFIED_POSITION: hold at specified position| MAV_FRAME coordinate frame of hold point| Desired yaw angle in degrees| Latitude / X position| Longitude / Y position| Altitude / Z position|  */
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, 0, 0, MAV_CMD_OVERRIDE_GOTO, 1, MAV_GOTO_DO_CONTINUE, 0, 0, 0, 0, 0, 0);
    mavlink->sendMessage(msg);
}

void QGCSwarmControl::stopButton_clicked()
{
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, 0, 0, MAV_CMD_OVERRIDE_GOTO, 1, MAV_GOTO_DO_HOLD, MAV_GOTO_HOLD_AT_CURRENT_POSITION, 0, 0, 0, 0, 0);
    mavlink->sendMessage(msg);
}

void QGCSwarmControl::armButton_clicked()
{
    if (modeIdx >= 0 && modeIdx < _modeList.count())
    {
        AutoPilotPluginManager::FullMode_t fullMode = _modeList[modeIdx];

        fullMode.baseMode |= MAV_MODE_FLAG_SAFETY_ARMED;

        if (ui->avoidanceBox->checkState() == Qt::Checked)
        {
            fullMode.baseMode |= MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
        }
        else
        {
            fullMode.baseMode &= ~MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
        }

        fullMode.customMode = 0;

        mavlink_message_t msg;
        mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, 0, 0, MAV_CMD_DO_SET_MODE, 1, fullMode.baseMode, fullMode.customMode, 0, 0, 0, 0, 0);
        mavlink->sendMessage(msg);
    }
}

void QGCSwarmControl::disarmButton_clicked()
{
    if (modeIdx >= 0 && modeIdx < _modeList.count())
    {
        AutoPilotPluginManager::FullMode_t fullMode = _modeList[modeIdx];

        fullMode.baseMode &= ~MAV_MODE_FLAG_SAFETY_ARMED;

        if (ui->avoidanceBox->checkState() == Qt::Checked)
        {
            fullMode.baseMode |= MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
        }
        else
        {
            fullMode.baseMode &= ~MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
        }
        fullMode.customMode = 0;

        mavlink_message_t msg;
        mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, 0, 0, MAV_CMD_DO_SET_MODE, 1, fullMode.baseMode, fullMode.customMode, 0, 0, 0, 0, 0);
        mavlink->sendMessage(msg);
    }
}

void QGCSwarmControl::selectButton_clicked()
{
    if (all_selected)
    {
        all_selected = false;
        ui->selectButton->setText(tr("Unselect all"));

        QMap<UASInterface*,QListWidgetItem*>::iterator it;
        for(it=uasToItemRemote.begin();it!=uasToItemRemote.end();++it)
        {
            it.value()->setCheckState(Qt::Checked);
        }
    }
    else
    {
        all_selected = true;
        ui->selectButton->setText(tr("Select all"));

        QMap<UASInterface*,QListWidgetItem*>::iterator it;
        for(it=uasToItemRemote.begin();it!=uasToItemRemote.end();++it)
        {
            it.value()->setCheckState(Qt::Unchecked);
        }
    }
}

void QGCSwarmControl::remoteItem_clicked(QListWidgetItem* item)
{
    if (item->checkState() == Qt::Checked)
    {
        item->setCheckState(Qt::Unchecked);
    }
    else
    {
        item->setCheckState(Qt::Checked);
    }
    
}

void QGCSwarmControl::remoteButton_clicked()
{
    mavlink_message_t msg;
    UASInterface* uas;
    int remoteVal;
    
    QMap<UASInterface*,QListWidgetItem*>::iterator it;
    for(it=uasToItemRemote.begin();it!=uasToItemRemote.end();++it)
    {
        uas = it.key();
        if(it.value()->checkState() == Qt::Checked)
        {
            remoteVal = 1;
        }
        else
        {
            remoteVal = 0;
        }
        mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, uas->getUASID(), MAV_COMP_ID_SYSTEM_CONTROL, MAV_CMD_DO_PARACHUTE, 1, remoteVal, 0, 0, 0, 0, 0, 0);
        mavlink->sendMessage(msg);
    }
}

void QGCSwarmControl::setMode(int mode)
{
    // Adapt context button mode
    modeIdx = mode;
    ui->modeComboBox->blockSignals(true);
    ui->modeComboBox->setCurrentIndex(mode);
    ui->modeComboBox->blockSignals(false);
}

void QGCSwarmControl::sendNewHomePosition()
{
    mavlink_message_t msg;

    double lat = UASManager::instance()->getHomeLatitude();
    double lon = UASManager::instance()->getHomeLongitude();
    double alt = UASManager::instance()->getHomeAltitude();

    //mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, 0, 0, MAV_CMD_DO_SET_HOME, 1, 0, 0, 0, 0, lat, lon, alt);
    
    mavlink_set_home_position_t home_msg;

    home_msg.latitude = lat * 10000000;
    home_msg.longitude = lon * 10000000;
    home_msg.altitude = alt * 1000;

    home_msg.x = 0.0f;
    home_msg.y = 0.0f;
    home_msg.z = 0.0f;

    home_msg.q[0] = 0.0f;
    home_msg.q[1] = 0.0f;
    home_msg.q[2] = 0.0f;
    home_msg.q[3] = 0.0f;

    home_msg.approach_x = 0.0f;
    home_msg.approach_y = 0.0f;
    home_msg.approach_z = 0.0f;

    mavlink_msg_set_home_position_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, 0,
                                        home_msg.latitude,
                                        home_msg.longitude,
                                        home_msg.altitude,
                                        home_msg.x,
                                        home_msg.y,
                                        home_msg.z,
                                        home_msg.q,
                                        home_msg.approach_x,
                                        home_msg.approach_y,
                                        home_msg.approach_z );

    mavlink->sendMessage(msg);
}

void QGCSwarmControl::strategyLaunchClicked()
{
    int strategy = ui->comboBoxStrategy->currentIndex()+1;


    mavlink_message_t msg;
    mavlink_msg_command_long_pack(mavlink->getSystemId(), mavlink->getComponentId(), &msg, 0, 0, MAV_CMD_NAV_ROI, 1, strategy, 0, 0, 0, 0, 0, 0);
    mavlink->sendMessage(msg);
}
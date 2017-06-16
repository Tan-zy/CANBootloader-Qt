#include "mainwindow.h"
#include "ui_mainwindow_ch.h"
#include <QDebug>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
   int ret;
    VCI_BOARD_INFO1 vci;
    timeout_flag = 0;
    ui->setupUi(this);
    ui->cmdListTableWidget->setColumnWidth(0,180);
    ui->cmdListTableWidget->setColumnWidth(1,180);
    for(int i=0;i<ui->cmdListTableWidget->rowCount();i++){
        ui->cmdListTableWidget->setRowHeight(i,35);
    }
    //qDebug << "sizeof(CANBaudRateTab) = " << sizeof(CANBaudRateTab);
    //检测是否有CAN连接
    ret = VCI_FindUsbDevice(&vci);
    if(ret <= 0)
    {
        USB_CAN_status = 1;
       // ui->deviceIndexComboBox->se

        ui->Connect_USB_CAN->setText(tr("无设备"));
    }
    else
    {
        ui->Connect_USB_CAN->setText(tr("连接CAN"));
        ui->deviceIndexComboBox->setMaxCount(ret);
    }
    //初始化部分按钮的状态
    ui->Close_CAN->setEnabled(false);
    ui->Connect_USB_CAN->setEnabled(true);
    ui->setbaudRatePushButton->setEnabled(false);
    ui->updateFirmwarePushButton->setEnabled(false);
    ui->newBaudRateComboBox->setEnabled(false);
    ui->allNodeCheckBox->setEnabled(false);
    ui->baudRateComboBox->setCurrentIndex(1);
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::Time_update()
{
    timeout_flag = 1;
  //  waitforreadyread()
    qDebug() << "timeout_flag = 1;";
}

void MainWindow::on_openFirmwareFilePushButton_clicked()
{
    QString fileName;
    fileName=QFileDialog::getOpenFileName(this,
                                          tr("Open files"),
                                          "",
                                          "Binary Files (*.bin);;Hex Files (*.hex);;All Files (*.*)");
    if(fileName.isNull()){
        return;
    }
    ui->firmwareLineEdit->setText(fileName);
}

int MainWindow::CAN_GetBaudRateNum(unsigned int BaudRate)
{
    for(int i=0;i<27;i++){
        if(BaudRate == CANBaudRateTab[i].BaudRate){
            return i;
        }
    }
    return 0;
}

bool MainWindow::DeviceConfig(void)
{

    return true;
}
void MainWindow::on_updateFirmwarePushButton_clicked()
{
    QTime time;
    time.start();
    int ret;
    bool ConfFlag;
    uint32_t appversion,appType;
    uint8_t FirmwareData[1026]={0};
    if(ui->allNodeCheckBox->isChecked())
    {
        if(ui->nodeListTableWidget->rowCount()<=0)
        {
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("无任何节点！"));
            return;
        }
    }
    else
        {
            if(ui->nodeListTableWidget->currentIndex().row()<0)
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("请选择节点！"));
                return;
             }
        }
    uint16_t NodeAddr;
     //USB_CAN_status = 1;
#if 1
    ConfFlag = DeviceConfig();
    if(!ConfFlag){
        return;
    }
#endif
    if(ui->allNodeCheckBox->isChecked()){
        NodeAddr = 0x00;
        ret = CAN_BL_Excute(ui->deviceIndexComboBox->currentIndex(),
                            ui->channelIndexComboBox->currentIndex(),
                            NodeAddr,
                            CAN_BL_BOOT);
        if(ret != CAN_SUCCESS)
        {
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("执行固件程序失败！"));
            USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
            return;
        }
        Sleep(500);
    }
    else
        {
        NodeAddr = ui->nodeListTableWidget->item(ui->nodeListTableWidget->currentIndex().row(),0)->text().toInt(NULL,16);
        ret = CAN_BL_NodeCheck(ui->deviceIndexComboBox->currentIndex(),
                            ui->channelIndexComboBox->currentIndex(),
                            NodeAddr,
                            &appversion,
                            &appType,
                            500);
        if(ret == CAN_SUCCESS){
            if(appType != CAN_BL_BOOT){//当前固件不为Bootloader
                ret = CAN_BL_Excute(ui->deviceIndexComboBox->currentIndex(),
                                    ui->channelIndexComboBox->currentIndex(),
                                    NodeAddr,
                                    CAN_BL_BOOT);
                if(ret != CAN_SUCCESS)
                   {
                        QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("执行固件程序失败！"));
                        USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
                        return;
                    }
                Sleep(500);
            }
        }
        else
        {
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("节点检测失败！"));
            USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
            return;
        }
    }
    QFile firmwareFile(ui->firmwareLineEdit->text());
    if (firmwareFile.open(QFile::ReadOnly)){
        if(!ui->allNodeCheckBox->isChecked()){
            ret = CAN_BL_NodeCheck(ui->deviceIndexComboBox->currentIndex(),
                                ui->channelIndexComboBox->currentIndex(),
                                NodeAddr,
                                &appversion,
                                &appType,
                                100);
            if(ret == CAN_SUCCESS)
            {
                if(appType != CAN_BL_BOOT)
                {
                    QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("当前固件不为Bootloader固件！"));
                    USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
                    return;
                }
            }
            else
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("节点检测失败！"));
                USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
                return;
            }
        }
        ret = CAN_BL_Erase(ui->deviceIndexComboBox->currentIndex(),
                           ui->channelIndexComboBox->currentIndex(),
                           NodeAddr,
                           firmwareFile.size(),
                           1000);
        if(ret != CAN_SUCCESS)
        {
            qDebug()<<"CBL_EraseFlash = "<<ret;
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("擦出Flash失败！"));
            USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
            return;
        }
        if(ui->allNodeCheckBox->isChecked())
        {
            Sleep(1000);
        }
        int read_data_num;
        QProgressDialog writeDataProcess(QStringLiteral("正在更新固件..."),QStringLiteral("取消"),0,firmwareFile.size(),this);
        writeDataProcess.setWindowTitle(QStringLiteral("更新固件"));
        writeDataProcess.setModal(true);
        writeDataProcess.show();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        int i=0;
        int PackSize = 512;
        for(i=0;i<firmwareFile.size();i+=PackSize){
            read_data_num = firmwareFile.read((char*)FirmwareData,PackSize);
            ret = CAN_BL_Write(ui->deviceIndexComboBox->currentIndex(),
                               ui->channelIndexComboBox->currentIndex(),
                               NodeAddr,
                               i,
                               FirmwareData,
                               read_data_num,
                               1000);
            if(ret != CAN_SUCCESS)
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("写Flash数据失败！"));
                USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
                return;
            }
            writeDataProcess.setValue(i);
            QCoreApplication::processEvents(QEventLoop::AllEvents);
            if(writeDataProcess.wasCanceled()){
                USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
                return;
            }
            if(ui->allNodeCheckBox->isChecked()){
                Sleep(10);
            }
        }
        writeDataProcess.setValue(firmwareFile.size());
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        if(writeDataProcess.wasCanceled()){
            USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
            return;
        }
    }
    else
        {
        USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
        QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("打开固件文件失败！"));
        return;
         }
    //执行固件
    ret = CAN_BL_Excute(ui->deviceIndexComboBox->currentIndex(),
                        ui->channelIndexComboBox->currentIndex(),
                        NodeAddr,
                        CAN_BL_APP);
    if(ret != CAN_SUCCESS)
    {
        QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("执行固件程序失败！"));

    }
    Sleep(50);
    if(!ui->allNodeCheckBox->isChecked()){
        ret = CAN_BL_NodeCheck(ui->deviceIndexComboBox->currentIndex(),
                            ui->channelIndexComboBox->currentIndex(),
                            NodeAddr,
                            &appversion,
                            &appType,
                            500);
        if(ret == CAN_SUCCESS){
            QString str;
            if(appType == CAN_BL_BOOT){
                str = "BOOT";
            }else{
                str = "APP";
            }
            ui->nodeListTableWidget->item(ui->nodeListTableWidget->currentIndex().row(),1)->setText(str);
            str.sprintf("v%d.%d",(((appversion>>24)&0xFF)*10)+(appversion>>16)&0xFF,(((appversion>>8)&0xFF)*10)+appversion&0xFF);
            ui->nodeListTableWidget->item(ui->nodeListTableWidget->currentIndex().row(),2)->setText(str);
        }else{
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("执行固件程序失败！"));
        }
    }
    USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
    qDebug()<<time.elapsed()/1000.0<<"s";
}

void MainWindow::on_openFirmwareFileAction_triggered()
{
    on_openFirmwareFilePushButton_clicked();
}


void MainWindow::on_scanNodeAction_triggered()
{
    int ret;
    int startAddr = 0,endAddr = 0;
    ScanDevRangeDialog *pScanDevRangeDialog = new ScanDevRangeDialog();
    if(pScanDevRangeDialog->exec() == QDialog::Accepted)
    {
        startAddr = pScanDevRangeDialog->StartAddr;
        endAddr = pScanDevRangeDialog->EndAddr;
    }
    else
    {
        return ;
    }
    if( USB_CAN_status != 0x04)
    {
          QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("打开设备失败！"));
          return ;
    }
    ui->nodeListTableWidget->verticalHeader()->hide();
    ui->nodeListTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->nodeListTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->nodeListTableWidget->setRowCount(0);
    QProgressDialog scanNodeProcess(QStringLiteral("正在扫描节点..."),QStringLiteral("取消"),0,endAddr-startAddr,this);
    scanNodeProcess.setWindowTitle(QStringLiteral("扫描节点"));
    scanNodeProcess.setModal(true);
    scanNodeProcess.show();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    int i=0;
    while(startAddr <= endAddr)
    {
        uint32_t appversion,appType;
        i++;
        /*
        ret = CAN_BL_NodeCheck(ui->deviceIndexComboBox->currentIndex(),
                            ui->channelIndexComboBox->currentIndex(),
                            startAddr,
                            &appversion,
                            &appType,
                            10);
                            */
        ret = CAN_BL_Nodecheck(ui->deviceIndexComboBox->currentIndex(),
                            ui->channelIndexComboBox->currentIndex(),
                            startAddr,
                            &appversion,
                            &appType,
                            10);
        if(ret == CAN_SUCCESS)
        {
            ui->nodeListTableWidget->setRowCount(ui->nodeListTableWidget->rowCount()+1);
            ui->nodeListTableWidget->setRowHeight(ui->nodeListTableWidget->rowCount()-1,20);
            QString str;
            str.sprintf("0x%X",startAddr);
            QTableWidgetItem *item = new QTableWidgetItem(str);
            ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,0,item);
            if(appType == CAN_BL_BOOT){
                     qDebug() << "appType=0x%x " << appType;
                //   qDebug << "appType=0x%x " << appType;
                str = "BOOT";
            }else{
                str = "APP";
                qDebug() << "appType=0x%x " << appType;
            }
            item = new QTableWidgetItem(str);
            ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,1,item);
            str.sprintf("v%d.%d",(((appversion>>24)&0xFF)*10)+(appversion>>16)&0xFF,(((appversion>>8)&0xFF)*10)+appversion&0xFF);
            item = new QTableWidgetItem(str);
            ui->nodeListTableWidget->setItem(ui->nodeListTableWidget->rowCount()-1,2,item);
        }
        scanNodeProcess.setValue(i);
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        if(scanNodeProcess.wasCanceled()){
            USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
            return;
        }
        startAddr++;
    }
    USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
}
/*********************************************
 * 是设定设备新的波特率,同时也需要设定USB_CAN的波特率
 *
 * */
void MainWindow::on_setbaudRatePushButton_clicked()
{

    int ret;


    if(ui->allNodeCheckBox->isChecked())
    {
        if(ui->nodeListTableWidget->rowCount()<=0)
        {
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("无任何节点！"));
            return;
        }
    }
    else
    {
        if(ui->nodeListTableWidget->currentIndex().row()<0)
        {
            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("请选择节点！"));
            return;
        }
    }
    /*
     *    CAN_INIT_CONFIG CAN_InitConfig;
    bool ConfFlag = DeviceConfig();
    if(!ConfFlag)
    {
        USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
        return;
    }
    QString str = ui->newBaudRateComboBox->currentText();
    str.resize(str.length()-4);
    int baud = str.toInt(NULL,10)*1000;
    CAN_InitConfig.CAN_BRP = CANBaudRateTab[CAN_GetBaudRateNum(baud)].PreScale;
    CAN_InitConfig.CAN_SJW = CANBaudRateTab[CAN_GetBaudRateNum(baud)].SJW;
    CAN_InitConfig.CAN_BS1 = CANBaudRateTab[CAN_GetBaudRateNum(baud)].BS1;
    CAN_InitConfig.CAN_BS2 = CANBaudRateTab[CAN_GetBaudRateNum(baud)].BS2;


    ret = CAN_BL_SetNewBaudRate(ui->deviceIndexComboBox->currentIndex(),
                                ui->channelIndexComboBox->currentIndex(),
                                NodeAddr,
                                &CAN_InitConfig,
                                baud,
                                100);
    if(ret != CAN_SUCCESS)
    {
        USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
        QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("设置波特率失败！"));
        return;
    }
    ui->baudRateComboBox->setCurrentIndex(ui->newBaudRateComboBox->currentIndex());
    USB_CloseDevice(ui->deviceIndexComboBox->currentIndex());
    */
    uint16_t NodeAddr;
    if(ui->allNodeCheckBox->isChecked())
    {
        NodeAddr = 0x00;
    }
    else
    {
        NodeAddr = ui->nodeListTableWidget->item(ui->nodeListTableWidget->currentIndex().row(),0)->text().toInt(NULL,16);
    }
   ret  = CAN_BL_erase(ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex(),NodeAddr,0x120,50);
    if(ret == 1)
        {
              qDebug() << "数据擦除成功";
        }
    else
        {
            qDebug() << "失败";
        }

}

void MainWindow::on_contactUsAction_triggered()
{
    QString AboutStr;
    AboutStr.append(("官方网站<span style=\"font-size:12px;\">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span>：<a href=\"http://www.usbxyz.com\">www.usbxyz.com</a><br>"));
    AboutStr.append(("官方论坛<span style=\"font-size:12px;\">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span>：<a href=\"http://www.embed-net.com\">www.embed-net.com</a><br>"));
    AboutStr.append(("官方淘宝店<span style=\"font-size:9px;\">&nbsp;&nbsp;&nbsp;</span>：<a href=\"http://usbxyz.taobao.com/\">usbxyz.taobao.com</a><br>"));
    AboutStr.append(("技术支持QQ：188298598<br>"));
    AboutStr.append(("产品咨询QQ：188298598"));
    QMessageBox::about(this,("联系我们"),AboutStr);
}

void MainWindow::on_aboutAction_triggered()
{
    QString AboutStr;
    AboutStr.append("USB2XXX USB2CAN Bootloader 1.0.1<br>");
    AboutStr.append(("支持硬件：USB2XXX<br>"));
    AboutStr.append(("购买地址：<a href=\"http://usbxyz.taobao.com/\">usbxyz.taobao.com</a>"));
    QMessageBox::about(this,("关于USB2XXX USB2CAN Bootloader"),AboutStr);
}

void MainWindow::on_exitAction_triggered()
{
    this->close();
}

void MainWindow::on_Connect_USB_CAN_clicked()
{
    int ret;
    bool state;
    state = VCI_OpenDevice(4,ui->deviceIndexComboBox->currentIndex(),0);
    if(!state)
    {
        QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("打开设备失败！"));
         USB_CAN_status = 2;
        return;
    }
    else
        {
            if(USB_CAN_status == 1)
                {
                    USB_CAN_status = 0;
                     ui->Connect_USB_CAN->setText(tr("连接CAN"));
                }

        }
    if(USB_CAN_status == 1)
        {

            QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("无设备连接！"));
        }

    CBL_CMD_LIST CMD_List;
    QString cmdStr[]={"Erase","WriteInfo","Write","Check","SetBaudRate","Excute","CmdSuccess","CmdFaild"};
    uint8_t cmdData[16];
    for(int i=0;i<ui->cmdListTableWidget->rowCount();i++)
    {
         ui->cmdListTableWidget->item(i,0)->setTextAlignment(Qt::AlignHCenter);
          ui->cmdListTableWidget->item(i,0)->setTextAlignment(Qt::AlignCenter);
         ui->cmdListTableWidget->item(i,1)->setTextAlignment(Qt::AlignHCenter);
          ui->cmdListTableWidget->item(i,1)->setTextAlignment(Qt::AlignCenter);
        if(ui->cmdListTableWidget->item(i,0)->text()==cmdStr[i])
        {
            cmdData[i] = ui->cmdListTableWidget->item(i,1)->text().toInt(NULL,16);
        }
    }
    CMD_List.Erase = cmdData[0];
    CMD_List.WriteInfo = cmdData[1];
    CMD_List.Write = cmdData[2];
    CMD_List.Check = cmdData[3];
    CMD_List.SetBaudRate = cmdData[4];
    CMD_List.Excute = cmdData[5];
    CMD_List.CmdSuccess = cmdData[6];
    CMD_List.CmdFaild = cmdData[7];
    CAN_BL_init(&CMD_List);//初始化cmd
    int baud_indx;
    baud_indx = ui->baudRateComboBox->currentIndex();
    VCI_INIT_CONFIG VCI_init;
    VCI_init.AccCode = 0x00000000;
    VCI_init.AccMask = 0xFFFFFFFF;
    VCI_init.Filter = 1;
    VCI_init.Mode = 0;
    VCI_init.Reserved = 0x00;
    VCI_init.Timing0  = CANBus_Baudrate_table[baud_indx].Timing0;//波特率的配置
    VCI_init.Timing1  = CANBus_Baudrate_table[baud_indx].Timing1;//波特率的配置
    if(ui->channelIndexComboBox->currentIndex() == 0||(ui->channelIndexComboBox->currentIndex() == 1))
        {
            ret = VCI_InitCAN(4,ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex(),&VCI_init);
            if(ret!=1)
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("配置设备失败！"));
                USB_CAN_status = 3;
                return;
            }
            ret = 0;
            ret = VCI_StartCAN(4,ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex());
            if(ret!=1)
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("配置设备失败 USB_CAN_status = 6 ！"));
                USB_CAN_status = 3;
                return;
            }
            ui->Connect_USB_CAN->setEnabled(false);
            ui->Close_CAN->setEnabled(true);
            ui->setbaudRatePushButton->setEnabled(true);
            ui->updateFirmwarePushButton->setEnabled(true);
             ui->newBaudRateComboBox->setEnabled(true);
             ui->allNodeCheckBox->setEnabled(true);
             ui->baudRateComboBox->setEnabled(false);
             ui->deviceIndexComboBox->setEnabled(false);
             ui->channelIndexComboBox->setEnabled(false);
             USB_CAN_status = 0x04;
             VCI_ClearBuffer(4,ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex());
        }
    else if (ui->channelIndexComboBox->currentIndex() == 2)
        {
            ret = VCI_InitCAN(4,ui->deviceIndexComboBox->currentIndex(),0,&VCI_init);
            if(ret!=1)
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("配置设备失败！"));
                USB_CAN_status = 3;
                return;
            }
            ret = VCI_StartCAN(4,ui->deviceIndexComboBox->currentIndex(),0);
            if(ret!=1)
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("配置设备失败 USB_CAN_status =4！"));
               USB_CAN_status = 3;
                return;
            }
            ret = VCI_InitCAN(4,ui->deviceIndexComboBox->currentIndex(),1,&VCI_init);
            if(ret!=1)
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("配置设备失败！"));
                USB_CAN_status = 3;
                return;
            }
            ret = VCI_StartCAN(4,ui->deviceIndexComboBox->currentIndex(),1);
            if(ret!=1)
            {
                QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("配置设备失败 USB_CAN_status =5！"));
                USB_CAN_status = 3;
                return;
            }
            ui->channelIndexComboBox->setDisabled(true);
            ui->channelIndexComboBox->setEnabled(false);
            ui->Connect_USB_CAN->setEnabled(false);
            ui->Close_CAN->setEnabled(true);
            ui->setbaudRatePushButton->setEnabled(true);
            ui->updateFirmwarePushButton->setEnabled(true);
            ui->newBaudRateComboBox->setEnabled(true);
            ui->allNodeCheckBox->setEnabled(true);
            ui->baudRateComboBox->setEnabled(false);
            ui->deviceIndexComboBox->setEnabled(false);
            ui->channelIndexComboBox->setEnabled(false);
             USB_CAN_status = 0x04;
             VCI_ClearBuffer(4,ui->deviceIndexComboBox->currentIndex(),0);
             VCI_ClearBuffer(4,ui->deviceIndexComboBox->currentIndex(),1);
        }

    }

void MainWindow::on_Close_CAN_clicked()
{
    int ret;
    if(ui->deviceIndexComboBox->currentIndex() != 2)
        {
            ret = VCI_ResetCAN(4,ui->deviceIndexComboBox->currentIndex(),ui->channelIndexComboBox->currentIndex());
            if(ret != 1)
                {
                    QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("复位设备失败！"));
                }
            else
                {
                     USB_CAN_status = 0;
                }
            ret = VCI_CloseDevice(4,ui->deviceIndexComboBox->currentIndex());
            if(ret != 1)
                {
                    QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("关闭设备失败！"));
                }
            else
                {
                     USB_CAN_status = 0;
                }
        }
    else if(ui->deviceIndexComboBox->currentIndex() == 2)
        {
            ret = VCI_ResetCAN(4,ui->deviceIndexComboBox->currentIndex(),0);
            if(ret != 1)
                {
                    QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("复位设备失败！"));
                }
            else
                {
                     USB_CAN_status = 0;
                }
            ret = VCI_ResetCAN(4,ui->deviceIndexComboBox->currentIndex(),1);
            if(ret != 1)
                {
                    QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("复位设备失败！"));
                }
            else
                {
                     USB_CAN_status = 0;
                }
            ret = VCI_CloseDevice(4,ui->deviceIndexComboBox->currentIndex());
            if(ret != 1)
                {
                    QMessageBox::warning(this,QStringLiteral("警告"),QStringLiteral("关闭设备失败！"));
                }
            else
                {
                     USB_CAN_status = 0;
                }
        }
    ui->setbaudRatePushButton->setEnabled(false);
    ui->updateFirmwarePushButton->setEnabled(false);
    ui->Connect_USB_CAN->setEnabled(true);
    ui->Close_CAN->setEnabled(false);
    ui->baudRateComboBox->setEnabled(true);
    ui->deviceIndexComboBox->setEnabled(true);
    ui->channelIndexComboBox->setEnabled(true);
    ui->newBaudRateComboBox->setEnabled(false);
    ui->allNodeCheckBox->setEnabled(false);
}
//------------------------------------------------------------------------
//以下函数是根据自己的CAN设备进行编写
int MainWindow::CAN_BL_Nodecheck(int DevIndex,int CANIndex,unsigned short NodeAddr,unsigned int *pVersion,unsigned int *pType,unsigned int TimeOut)
{
    int ret = 0;
    int read_num = 0;
    bootloader_data Bootloader_data ;
    VCI_ClearBuffer(4,DevIndex,CANIndex);
    VCI_CAN_OBJ can_send_msg,can_read_msg[1000];
    Bootloader_data.ExtId.bit.reserve = 0x00;
    Bootloader_data.ExtId.bit.cmd = cmd_list.Check;
    Bootloader_data.ExtId.bit.addr = NodeAddr;
    can_send_msg.DataLen = 0;
    can_send_msg.SendType = 1;
    can_send_msg.RemoteFlag = 0;
    can_send_msg.ExternFlag = 1;
    can_send_msg.ID = Bootloader_data.ExtId.all;
    ret =  VCI_Transmit(4,DevIndex,CANIndex,&can_send_msg,1);
    if(ret == -1)
    {
     return 1;
    }
    QTimer::singleShot(TimeOut, this, &MainWindow::Time_update);
    if(timeout_flag == 1)
        {
            timeout_flag = 0;
             read_num  =VCI_GetReceiveNum(4,DevIndex,CANIndex);
        }
        else
        {
            read_num = 0;
        }

    if(read_num == 0)
        {
            return 1;
        }
    else if(read_num == -1)
        {
            return 1;
        }
    else
        {
            ret = VCI_Receive(4,DevIndex,CANIndex,&can_read_msg[0],1000,0);
            if(ret == -1)
            {
                return 1;
                VCI_ClearBuffer(4,DevIndex,CANIndex);
            }
            if(ret == 1)
            {
                *pVersion = can_read_msg[0].Data[0]<<24|can_read_msg[0].Data[1]<<16|can_read_msg[0].Data[2]<<8|can_read_msg[0].Data[3]<<0;
                *pType =    can_read_msg[0].Data[4]<<24|can_read_msg[0].Data[5]<<16|can_read_msg[0].Data[6]<<8|can_read_msg[0].Data[7]<<0;
            }
        }

    VCI_ClearBuffer(4,DevIndex,CANIndex);
    return 0;
}
int MainWindow::CAN_BL_init(PCBL_CMD_LIST pCmdList)
{

    cmd_list.Check      = pCmdList->Check;
    cmd_list.Erase      = pCmdList->Erase;
    cmd_list.Excute     = pCmdList->Excute;
    cmd_list.Write      = pCmdList->Write;
    cmd_list.WriteInfo  = pCmdList->WriteInfo;
    cmd_list.CmdFaild   = pCmdList->CmdFaild;
    cmd_list.CmdSuccess = pCmdList->CmdSuccess;
    return 0;
}

int   MainWindow::CAN_BL_erase(int DevIndex,int CANIndex,unsigned short NodeAddr,unsigned int FlashSize,unsigned int TimeOut)
{
      //  WaitCommEvent()
    int ret;
    bootloader_data Bootloader_data ;
    int i,read_num;
    VCI_CAN_OBJ can_send_msg,can_read_msg[1000];
    Bootloader_data.DLC = 4;
    Bootloader_data.ExtId.bit.cmd = cmd_list.Erase;
    Bootloader_data.ExtId.bit.addr = NodeAddr;
    Bootloader_data.ExtId.bit.reserve = 0;
    Bootloader_data.IDE = CAN_ID_EXT;
    Bootloader_data.data[0] = ( FlashSize & 0xFF000000 ) >> 24;
    Bootloader_data.data[1] = ( FlashSize & 0xFF0000 ) >> 16;
    Bootloader_data.data[2] = ( FlashSize & 0xFF00 ) >> 8;
    Bootloader_data.data[3] = ( FlashSize & 0x00FF );
    VCI_ClearBuffer(4,DevIndex,CANIndex);
    can_send_msg.DataLen = Bootloader_data.DLC;
    can_send_msg.SendType = 1;
    can_send_msg.RemoteFlag = 0;
    can_send_msg.ExternFlag = Bootloader_data.IDE;
    can_send_msg.ID = Bootloader_data.ExtId.all;
    for(i = 0;i<Bootloader_data.DLC;i++)
        {
            can_send_msg.Data[i] = Bootloader_data.data[i];
        }
    ret =  VCI_Transmit(4,DevIndex,CANIndex,&can_send_msg,1);
    if(ret == -1)
    {
     return 1;
    }
    QTimer::singleShot(TimeOut, this, &MainWindow::Time_update);
    if(timeout_flag == 1)
        {
            timeout_flag = 0;
             read_num  =VCI_GetReceiveNum(4,DevIndex,CANIndex);
        }
        else
        {
            read_num = 0;
        }

    if(read_num == 0)
        {
            return 1;
        }
    else if(read_num == -1)
        {
            return 1;
        }
    else
        {
            ret = VCI_Receive(4,DevIndex,CANIndex,&can_read_msg[0],1000,0);
            if(ret == -1)
            {
                return 1;
                VCI_ClearBuffer(4,DevIndex,CANIndex);
            }
            if(ret == 1)
            {
//判断返回结果
                if(can_read_msg[0].ID == (Bootloader_data.ExtId.bit.addr<<4|cmd_list.CmdSuccess))//表示反馈数据有效
                    {
                        return 0;
                        qDebug()<<"成功擦除数据";
                    }
                    else
                    {
                        qDebug()<<"数据无效";
                        return 1;
                    }

            }
        }

    VCI_ClearBuffer(4,DevIndex,CANIndex);
    return 0;
}

int   MainWindow::CAN_BL_write(int DevIndex,int CANIndex,unsigned short NodeAddr,unsigned int AddrOffset,unsigned char *pData,unsigned int DataNum,unsigned int TimeOut)
{
    return 0;
}
int   CAN_BL_excute(int DevIndex,int CANIndex,unsigned short NodeAddr,unsigned int Type)
{
    return 0;
}

void MainWindow::on_action_Open_CAN_triggered()
{
on_Connect_USB_CAN_clicked();
}

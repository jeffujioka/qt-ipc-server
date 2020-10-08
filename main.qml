import QtQuick 2.13
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Window 2.13

import de.com.fujioka 1.0

Window {
    id: mainWindow
    width: 640
    height: 480
    visible: true
    title: qsTr("IPC Server")

    IpcServer {
        id: ipcServer
    }

    ColumnLayout {
        id: mainLayout
        visible: true

        GroupBox {
            id: gridBox

            GridLayout {
                rows: 2
                columns: 2
                flow: GridLayout.TopToBottom
                anchors.fill: parent

                Label {
                    id: srvNameLbl
                    text: "server name: "
                }
                Label {
                    id: fileNameLbl
                    text: "file name: "
                }

                TextField {
                    id: srvNameTxt
                    text: "mbition"
                }
                TextField {
                    id: fileNameTxt
                    text: "log.txt"
                }

            }
        }

        GroupBox {
            GridLayout {
                rows: 1
                columns: 2

                Button {
                    id: quitExit
                    text: "quit"

                    onClicked: close()
                }

                Button {
                    id: startBtn
                    text: "start"

                    onClicked: {
                        if (srvNameTxt.text == "") {
                            srvNameTxt.text = "mbition"
                        }
                        if (fileNameTxt.text == "") {
                            fileNameTxt.text = "log.txt"
                        }

                        ipcServer.setFileName(fileNameTxt.text)
                        ipcServer.startListening(srvNameTxt.text)
                        startBtn.enabled = false;
                    }
                }
            }
        }
    }
}

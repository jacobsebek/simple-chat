#include <wx/wx.h>
#include <wx/thread.h>
#include <wx/splitter.h>
#include <wx/tokenzr.h>
#include <wx/valnum.h>

extern "C" {
    #include "client.h"
}

class ChatFrontend : public wxApp
{

    // Our window
    wxFrame* frame;

    // The input and output boxes
    wxTextCtrl* input_box;
    wxTextCtrl* output_box;

    // The two buttons
    wxButton* send_button;
    wxButton* connect_button;

    // The static box that displays our name
    wxStaticBox* label_box;	

    // A boolean indicating if we are currently connected to a remote server
    bool connected = false;

    // Change the visual status, including the connected flag
    void SetStatusConnected(bool connected, const wxString& host=wxEmptyString) {

        this->connected = connected;

        wxString title;

        if (connected) {
            if (!host.IsEmpty())
                title = wxString::Format("Connected to '%s'", host);
            else
                title = wxString("Connected");
        } else {
            title = wxString("Not connected");
            label_box->SetLabel(wxEmptyString); 
        }

        frame->SetTitle(wxString::Format("Jacob's chat client - %s", title));        
    }

	void ConstructGui() {

        // This panel seems redunant but it's the preferred way of placing controls into a window
        // It enables the usage of TAB in the window and a natural bacground color
        wxPanel* panel;
        wxBoxSizer* panel_sizer = new wxBoxSizer(wxHORIZONTAL);
        panel_sizer->Add(panel = new wxPanel(frame), 1, wxEXPAND);

		wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

        // Add the output box first
		main_sizer->Add(
			output_box = new wxTextCtrl (panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH | wxHSCROLL),
			wxSizerFlags(1).Expand().Border(wxALL)
		);

        // The input sizer is a horizontal sizer, it contains the elements: INPUT_BOX CONNECT SEND
		wxBoxSizer* input_sizer = new wxStaticBoxSizer(label_box = new wxStaticBox(panel, wxID_ANY, ""), wxHORIZONTAL);
        // Add the input box
		input_sizer->Add(
			input_box = new wxTextCtrl (panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER | wxTE_MULTILINE | wxTE_RICH),
			wxSizerFlags(1).Expand().Border(wxALL)	
		);

        // Add the two buttons
		input_sizer->Add(
			connect_button = new wxButton(panel, wxID_ANY, "Connect"),
			wxSizerFlags(0).Expand().Border(wxALL)	
		);
		input_sizer->Add(
			send_button = new wxButton(panel, wxID_ANY, "Send"),
			wxSizerFlags(0).Expand().Border(wxALL)	
		);

        // Finally place the input sizer beneath the output box
		main_sizer->Add(
			input_sizer,
			wxSizerFlags(0).Expand()
		);

        panel->SetSizer(main_sizer);

        // TODO: put panel into panel sizer
		frame->SetSizer(panel_sizer);
		frame->SetMinSize(wxSize(320, 240));
		frame->SetSize(wxSize(640, 480));

        // Set the fonts
        // TELETYPE = monospace
        {
            wxFont font(wxFontInfo().Family(wxFONTFAMILY_TELETYPE).AntiAliased());
            input_box->SetFont(font);
            output_box->SetFont(font);
        }
	
		//bind the events
        // .. For the two buttons
		send_button->Bind(wxEVT_BUTTON, [&](wxCommandEvent &evt) { 
            (void)evt;

            Send(); 
        });

		connect_button->Bind(wxEVT_BUTTON, [&](wxCommandEvent &evt) { 
            (void)evt;

            Connect(); 
        });

        // In the input box, we make it easy to send messages by pressing enter
        // Notice that shift+enter or any other combination creates \v is ignored
        // as we allow multiline text
		input_box->Bind(wxEVT_TEXT_ENTER, [&](wxCommandEvent& evt) { 
            (void)evt;

			wxString str = input_box->GetValue();
			if (str.Last() != '\v') {
					
				// Remove the unwanted newline character or whatever
				int len = str.Len();
				input_box->Remove(len-1, len);

				Send();
			}

		});

        // Bind the idle event to receive, this is crucial
        // We don't have to use a separate thread, instead, this non-blocking function gets called whenever
        // The main thread is idle
        Bind(wxEVT_IDLE, Receive, this);

	}

	bool OnInit() override {
		frame = new wxFrame(NULL, wxID_ANY, wxEmptyString);

		//construct the gui
		ConstructGui();
        // Set our focus to the input box so we can start typing right away
        input_box->SetFocus();
        // Initialise the status - this sets the title to "not connected" etc.
        SetStatusConnected(false);
        // When we are done, whow the frame
		frame->Show(true);

		//initialize the network
		if (client_init() != CLIENT_ERR_OK) {
            wxMessageBox("Failed to initialise the client", "Fatal error", wxOK | wxICON_ERROR);
            return false;
        }

        // Immediately prompt the confused user to connect
        wxBell();
		Connect();

		return true;
	}

    int OnExit() override {
        // Make sure to deinit the client, this closes all the sockets and the underlying backend
        client_deinit();

        return 0;
    }

    // This function is called whenever a fatal error on the connection occurs
    void LostConnection() {
        SetStatusConnected(false);
        output_box->Clear();
        wxMessageBox("Lost connection to remote host", "Error", wxOK | wxICON_ERROR);
    }

    // Prints a properly formatted message Msg by Sender
    void PrintMsg(const wxString& sender, const wxString& msg) {

        wxString output = wxString::Format("[%s] <%s> : ", wxDateTime::Now().FormatISOCombined(' '), sender);
        *output_box << output;

        for (size_t i = 0; i < msg.Len(); i++) {
            *output_box << wxString(msg[i]);

            // This ensures that the text doesn't appear at the beginning of the next line but correctly aligned
            // [TIME] <nick> : like
            //                 this
            char c;
            if (msg[i].GetAsChar(&c) && (c == '\v' || c == '\n' || c == '\r'))
                *output_box << std::string(output.Len(), ' ');
        }
        *output_box << "\n";
    }

    // Sends the current contents of the input box to the moon
	void Send() {

        // Refuse to send if the message is empty or not connected
		if (!connected || input_box->IsEmpty()) {
			wxBell();
			return;
		}

        wxString input = input_box->GetValue();

		// send the message to the server
        struct client_msg msg;
        msg.type = CLIENT_MSG_MSG;
        msg.u.send.msg.text = const_cast<char*>(input.utf8_str().data());

        int status = client_send(&msg);
		if (status != CLIENT_ERR_OK) {

            if (status < 0)
                LostConnection();
            else
                wxMessageBox("Invalid message", "Failed to send message", wxOK | wxICON_ERROR);

            return;
        }

		// print the message to the output box
		PrintMsg(label_box->GetLabel(), input);

		input_box->Clear();
		input_box->SetFocus();	// set focus back if the send button was clicked
	}

	void Connect() {

        // Create dialog
        // IPDialog is a very simple dialog that prompts the user for the IP, Port, and a Nickname
        class IPDialog : public wxDialog {
            private:
                wxTextCtrl* input_IP;
                wxTextCtrl* input_port;
                wxTextCtrl* input_nickname;

            public:

            IPDialog (wxWindow* parent, const wxString& title) : wxDialog(parent, wxID_ANY, title) {

                // This ensures that when we close the dialog, it's going to abort
                // However, pressing ESC doesn't work with this setup without an actual Cancel button..
                SetEscapeId(wxID_CANCEL);
                
                // Create and populate the IP address + port prompt
                wxFlexGridSizer* hostinfo_sizer = new wxFlexGridSizer(2, 2, wxSizerFlags::GetDefaultBorder(), wxSizerFlags::GetDefaultBorder());
                hostinfo_sizer->AddGrowableCol(0);

                hostinfo_sizer->Add( new wxStaticText(this, wxID_ANY, "IP Address"));
                hostinfo_sizer->Add( new wxStaticText(this, wxID_ANY, "Port"));
                hostinfo_sizer->Add( input_IP = new wxTextCtrl(this, wxID_ANY), 0, wxEXPAND);
                hostinfo_sizer->Add( input_port = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(input_IP->GetSize().GetWidth()/2, input_IP->GetSize().GetHeight())));

                // Create and populate the Nick + Confirm button prompt
                wxFlexGridSizer* userinfo_sizer = new wxFlexGridSizer(2, 1, wxSizerFlags::GetDefaultBorder(), wxSizerFlags::GetDefaultBorder());
                userinfo_sizer->AddGrowableCol(0);

                userinfo_sizer->Add(new wxStaticText(this, wxID_ANY, "Nickname"));
                userinfo_sizer->Add(input_nickname = new wxTextCtrl(this, wxID_ANY));

                wxBoxSizer* lower_sizer = new wxBoxSizer(wxHORIZONTAL);
                lower_sizer->Add(
                    userinfo_sizer,
                    wxSizerFlags(1).Expand().Border(wxRIGHT)
                );
                wxButton* confirm_button;
                lower_sizer->Add(
                    confirm_button = new wxButton(this, wxID_OK, "Confirm"),
                    wxSizerFlags(0).Expand()
                );

                // Make the final sizer that contains all of the previous
                wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
                main_sizer->Add(hostinfo_sizer, wxSizerFlags().Expand().Border());
                main_sizer->Add(lower_sizer, wxSizerFlags().Expand().Border());

                // And set it to the window
                SetSizerAndFit(main_sizer);
 
            }

            wxString GetIP() {
                return input_IP->GetValue();
            }

            unsigned short GetPort() {
                return wxAtoi(input_port->GetValue());
            }

            wxString GetNick() {
                return input_nickname->GetValue();
            }
        };

        IPDialog dialog(frame, "Connect to remote server");

		if (dialog.ShowModal() != wxID_OK)
            return;

        // If anything in the future fails, we are disconnected
        SetStatusConnected(false);
        output_box->Clear();

        // 10 seconds connection timeout
		if (client_connect(dialog.GetIP(), dialog.GetPort(), 10000) != CLIENT_ERR_OK) {
            wxMessageBox("Failed to connect to remote host", "Error", wxOK | wxICON_ERROR);
            return;
        } 

        // Once we are connected, request to change our nick
        {
            wxString newnick = dialog.GetNick();

            struct client_msg nick_msg;
            nick_msg.type = CLIENT_MSG_NICK; 
            nick_msg.u.send.nick.newnick = const_cast<char*>(newnick.utf8_str().data());

            int status = client_send(&nick_msg);
            if (status != CLIENT_ERR_OK) {
                if (status < 0) {
                    LostConnection();
                    return;
                } else {
                    wxMessageBox("Invalid nick", "Failed to change nick", wxOK | wxICON_ERROR);
                }
            }
        }

        // If we get *all the way* here, we have passed and are officially connected
        SetStatusConnected(true, wxString::Format("%s:%d", dialog.GetIP(), dialog.GetPort()));
	}

    // A non-blocking Idle event handler
    // Processes all the pending messages
    //TODO: this could maybe be better if implemented as a timer - called a fixed amount of times per second
    void Receive(wxIdleEvent& evt) {
        if (!connected) return;

        // Ensure that we get a constant stream of these events
        evt.RequestMore(); 

        // Non-blocking IO : process all pending messages
        // Loop until there is stuff to read (until the "NO RECEIVE(D)" return value isn't returned)
        struct client_msg msg;
        int status;
        // Introduce a couple milliseconds of delay (timeout) to relax the CPU load
        // This won't have basicallly any negative effect
        while ((status = client_receive(&msg, 5)) != CLIENT_ERR_NOREC) {

            // If any error occurs, the client automatically disconnects, therefore update the gui too
            if (status != CLIENT_ERR_OK) {
                LostConnection();
                return;
            }

            switch (msg.type) {
                case CLIENT_MSG_MSG :
                    PrintMsg(wxString::FromUTF8(msg.u.rec.msg.sender), wxString::FromUTF8(msg.u.rec.msg.text));

                    // Don't forget that all strings in the client_msg structure returned by client_receive() are malloc-ated!
                    free(msg.u.rec.msg.sender);
                    free(msg.u.rec.msg.text);
                break;
                case CLIENT_MSG_NICK : 
                    label_box->SetLabel(wxString::FromUTF8(msg.u.rec.nick.newnick)); 

                    free(msg.u.rec.nick.newnick);
                break; 
                default:

                break;
            }

        }

    }

};

wxIMPLEMENT_APP(ChatFrontend);

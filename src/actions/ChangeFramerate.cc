/*
 *	subtitleeditor -- a tool to create or edit subtitle
 *
 *	http://home.gna.org/subtitleeditor/
 *	https://gna.org/projects/subtitleeditor/
 *
 *	Copyright @ 2005-2008, kitone
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <gtkmm.h>
#include "Document.h"
#include "Plugin.h"
#include "utility.h"
#include "DocumentSystem.h"
#include "gui/DialogUtility.h"


class DialogChangeFramerate : public DialogActionMultiDoc
{
	/*
	 *
	 */
	class ComboBoxEntryText : public Gtk::ComboBoxEntryText
	{
	public:
		ComboBoxEntryText(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade)
		:Gtk::ComboBoxEntryText(cobject)
		{
			get_entry()->signal_focus_out_event().connect(
					sigc::mem_fun(*this, &ComboBoxEntryText::on_focus_out));
		}

		void append_text(const Glib::ustring &text)
		{
			Gtk::TreeNodeChildren rows = get_model()->children();
			
			Gtk::ComboBoxEntryText::TextModelColumns m_columns;

			for(Gtk::TreeIter it = rows.begin(); it; ++it)
			{
				if((*it)[m_columns.m_column] == text)
				{
					set_active(it);
					return;
				}
			}

			Gtk::ComboBoxEntryText::append_text(text);
		}

		bool on_focus_out(GdkEventFocus* ev)
		{
			bool state = true;//get_entry()->on_focus_out_event(ev);

			Glib::ustring text = get_entry()->get_text();

			double value = 0;
			if(from_string(text, value))
			{
				if(value > 0)
					append_text(to_string(value));
				else
					set_active(0);
			}
			else
				set_active(0);

			return state;
		}
	};

public:

	/*
	 *
	 */
	DialogChangeFramerate(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade)
	:DialogActionMultiDoc(cobject, refGlade)
	{
		refGlade->get_widget_derived("combo-src", m_comboSrc);
		refGlade->get_widget_derived("combo-dest", m_comboDest);

		m_comboSrc->get_entry()->signal_activate().connect(
				sigc::bind<ComboBoxEntryText*>(
					sigc::mem_fun(*this, &DialogChangeFramerate::combo_activate), m_comboSrc));
		m_comboDest->get_entry()->signal_activate().connect(
				sigc::bind<ComboBoxEntryText*>(
					sigc::mem_fun(*this, &DialogChangeFramerate::combo_activate), m_comboDest));
	
		m_comboSrc->append_text(to_string(23.976));
		m_comboSrc->append_text(to_string(25.000));
		m_comboSrc->append_text(to_string(29.970));

		m_comboDest->append_text(to_string(23.976));
		m_comboDest->append_text(to_string(25.000));
		m_comboDest->append_text(to_string(29.970));

		m_comboSrc->set_active(0);
		m_comboDest->set_active(1);

		set_default_response(Gtk::RESPONSE_OK);
	}

	/*
	 *
	 */
	void execute()
	{
		show();

		if(run() == Gtk::RESPONSE_OK)
		{
			DocumentList docs;

			if(apply_to_all_documents())
				docs = DocumentSystem::getInstance().getAllDocuments();
			else
			{
				Document *doc = DocumentSystem::getInstance().getCurrentDocument();
				docs.push_back(doc);
			}

			double src = get_value(m_comboSrc);
			double dest = get_value(m_comboDest);
			
			if(src != 0 && dest != 0)
			{
				for(DocumentList::const_iterator it = docs.begin(); it!= docs.end(); ++it)
				{
					signal_change_framerate(*it, src, dest);
				}
			}
		}

		hide();
	}

	/*
	 *
	 */
	sigc::signal<void, Document*, double, double> signal_change_framerate;
protected:

	/*
	 *
	 */
	double get_value(ComboBoxEntryText *combo)
	{
		Glib::ustring text = combo->get_active_text();
		
		double value = 0;

		if(from_string(text, value))
			return value;
		return 0;
	}
	/*
	 *
	 */
	void combo_activate(ComboBoxEntryText *combo)
	{
		Glib::ustring text = combo->get_entry()->get_text();

		double value = 0;
		if(from_string(text, value))
		{
			if(value > 0)
			{
				combo->append_text(to_string(value));
				combo->set_active_text(to_string(value));
				return;
			}
		}

		combo->set_active(0);
	}

protected:
	ComboBoxEntryText* m_comboSrc;
	ComboBoxEntryText* m_comboDest;
};



/*
 *
 */
class ChangeFrameratePlugin : public Plugin
{
public:

	/*
	 *
	 */
	void activate()
	{
		se_debug(SE_DEBUG_PLUGINS);

		// actions
		action_group = Gtk::ActionGroup::create("ChangeFrameratePlugin");

		action_group->add(
				Gtk::Action::create("change-framerate", Gtk::Stock::CONVERT, _("Change _Framerate"), _("Convert framerate")),
					sigc::mem_fun(*this, &ChangeFrameratePlugin::on_execute));

		// ui
		Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

		ui_id = ui->new_merge_id();

		ui->insert_action_group(action_group);

		//ui->add_ui(ui_id, "/menubar/menu-timings/extend-6", "change-framerate", "change-framerate");
	}

	/*
	 *
	 */
	void deactivate()
	{
		se_debug(SE_DEBUG_PLUGINS);

		Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

		ui->remove_ui(ui_id);
		ui->remove_action_group(action_group);
	}

	/*
	 *
	 */
	void update_ui()
	{
		se_debug(SE_DEBUG_PLUGINS);

		bool visible = (get_current_document() != NULL);

		action_group->get_action("change-framerate")->set_sensitive(visible);
	}

protected:

	/*
	 *
	 */
	void on_execute()
	{
		se_debug(SE_DEBUG_PLUGINS);

		execute();
	}

	/*
	 *
	 */
	bool execute()
	{
		se_debug(SE_DEBUG_PLUGINS);

		Document *doc = get_current_document();

		g_return_val_if_fail(doc, false);

		// create dialog
		DialogChangeFramerate *dialog = utility::get_widget_derived<DialogChangeFramerate>("dialog-change-framerate.glade", "dialog-change-framerate");

		g_return_val_if_fail(dialog, false);

		dialog->signal_change_framerate.connect(
				sigc::mem_fun(*this, &ChangeFrameratePlugin::change_framerate));

		dialog->execute();

		delete dialog;

		return true;
	}
	

	/*
	 *
	 */
	void change_framerate(Document *doc, double src_fps, double dest_fps)
	{
		se_debug(SE_DEBUG_PLUGINS);

		g_return_if_fail(doc);
		
		doc->start_command(_("Change Framerate"));

		Subtitles subtitles = doc->subtitles();
	
		Subtitle subtitle = subtitles.get_first();

		while(subtitle)
		{
			SubtitleTime start = change_fps(subtitle.get_start(), src_fps, dest_fps);
			SubtitleTime end = change_fps(subtitle.get_end(), src_fps, dest_fps);

			subtitle.set_start_and_end(start, end);

			++subtitle;
		}
	
		doc->emit_signal("subtitle-time-changed");
		doc->finish_command();

		doc->flash_message(_("The new framerate was applied. (%s to %s)"), to_string(src_fps).c_str(), to_string(dest_fps).c_str());
	}

	/*
	 *
	 */
	SubtitleTime change_fps(const SubtitleTime &time, double src, double dest)
	{
		se_debug(SE_DEBUG_PLUGINS);

		double frame = time.totalmsecs * src;
		double tot_sec = frame / dest;

		return SubtitleTime((long)tot_sec);
	}

protected:
	Gtk::UIManager::ui_merge_id ui_id;
	Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_PLUGIN(ChangeFrameratePlugin)
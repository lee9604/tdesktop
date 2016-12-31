/*
This file is part of Telegram Desktop,
the official desktop version of Telegram messaging app, see https://telegram.org

Telegram Desktop is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

It is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

In addition, as a special exception, the copyright holders give permission
to link the code of portions of this program with the OpenSSL library.

Full license: https://github.com/telegramdesktop/tdesktop/blob/master/LICENSE
Copyright (c) 2014-2016 John Preston, https://desktop.telegram.org
*/
#include "stdafx.h"
#include "boxes/confirmbox.h"

#include "styles/style_boxes.h"
#include "lang.h"
#include "mainwidget.h"
#include "mainwindow.h"
#include "apiwrap.h"
#include "application.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/toast/toast.h"
#include "core/click_handler_types.h"
#include "localstorage.h"

TextParseOptions _confirmBoxTextOptions = {
	TextParseLinks | TextParseMultiline | TextParseRichText, // flags
	0, // maxw
	0, // maxh
	Qt::LayoutDirectionAuto, // dir
};

ConfirmBox::ConfirmBox(QWidget*, const QString &text, base::lambda<void()> &&confirmedCallback, base::lambda<void()> &&cancelledCallback)
: _confirmText(lang(lng_box_ok))
, _cancelText(lang(lng_cancel))
, _confirmStyle(st::defaultBoxButton)
, _text(st::boxWidth - st::boxPadding.left() - st::boxButtonPadding.right())
, _confirmedCallback(std_::move(confirmedCallback))
, _cancelledCallback(std_::move(cancelledCallback)) {
	init(text);
}

ConfirmBox::ConfirmBox(QWidget*, const QString &text, const QString &confirmText, base::lambda<void()> &&confirmedCallback, base::lambda<void()> &&cancelledCallback)
: _confirmText(confirmText)
, _cancelText(lang(lng_cancel))
, _confirmStyle(st::defaultBoxButton)
, _text(st::boxWidth - st::boxPadding.left() - st::boxButtonPadding.right())
, _confirmedCallback(std_::move(confirmedCallback))
, _cancelledCallback(std_::move(cancelledCallback)) {
	init(text);
}

ConfirmBox::ConfirmBox(QWidget*, const QString &text, const QString &confirmText, const style::RoundButton &confirmStyle, base::lambda<void()> &&confirmedCallback, base::lambda<void()> &&cancelledCallback)
: _confirmText(confirmText)
, _cancelText(lang(lng_cancel))
, _confirmStyle(confirmStyle)
, _text(st::boxWidth - st::boxPadding.left() - st::boxButtonPadding.right())
, _confirmedCallback(std_::move(confirmedCallback))
, _cancelledCallback(std_::move(cancelledCallback)) {
	init(text);
}

ConfirmBox::ConfirmBox(QWidget*, const QString &text, const QString &confirmText, const QString &cancelText, base::lambda<void()> &&confirmedCallback, base::lambda<void()> &&cancelledCallback)
: _confirmText(confirmText)
, _cancelText(cancelText)
, _confirmStyle(st::defaultBoxButton)
, _text(st::boxWidth - st::boxPadding.left() - st::boxButtonPadding.right())
, _confirmedCallback(std_::move(confirmedCallback))
, _cancelledCallback(std_::move(cancelledCallback)) {
	init(text);
}

ConfirmBox::ConfirmBox(QWidget*, const QString &text, const QString &confirmText, const style::RoundButton &confirmStyle, const QString &cancelText, base::lambda<void()> &&confirmedCallback, base::lambda<void()> &&cancelledCallback)
: _confirmText(confirmText)
, _cancelText(cancelText)
, _confirmStyle(st::defaultBoxButton)
, _text(st::boxWidth - st::boxPadding.left() - st::boxButtonPadding.right())
, _confirmedCallback(std_::move(confirmedCallback))
, _cancelledCallback(std_::move(cancelledCallback)) {
	init(text);
}

ConfirmBox::ConfirmBox(const InformBoxTag &, const QString &text, const QString &doneText, base::lambda_copy<void()> &&closedCallback)
: _confirmText(doneText)
, _confirmStyle(st::defaultBoxButton)
, _informative(true)
, _text(st::boxWidth - st::boxPadding.left() - st::boxButtonPadding.right())
, _confirmedCallback(generateInformCallback(closedCallback))
, _cancelledCallback(generateInformCallback(closedCallback)) {
	init(text);
}

base::lambda<void()> ConfirmBox::generateInformCallback(const base::lambda_copy<void()> &closedCallback) {
	auto callback = closedCallback;
	return base::lambda_guarded(this, [this, callback] {
		closeBox();
		if (callback) {
			callback();
		}
	});
}

void ConfirmBox::init(const QString &text) {
	_text.setText(st::boxTextStyle, text, _informative ? _confirmBoxTextOptions : _textPlainOptions);
}

void ConfirmBox::prepare() {
	addButton(_confirmText, [this] { confirmed(); }, _confirmStyle);
	if (!_informative) {
		addButton(_cancelText, [this] { _cancelled = true; closeBox(); });
	}
	textUpdated();
}

void ConfirmBox::textUpdated() {
	_textWidth = st::boxWidth - st::boxPadding.left() - st::boxButtonPadding.right();
	_textHeight = qMin(_text.countHeight(_textWidth), 16 * int(st::boxTextStyle.lineHeight));
	setDimensions(st::boxWidth, st::boxPadding.top() + _textHeight + st::boxPadding.bottom());

	setMouseTracking(_text.hasLinks());
}

void ConfirmBox::closeHook() {
	if (!_confirmed && (!_strictCancel || _cancelled) && _cancelledCallback) {
		_cancelledCallback();
	}
}

void ConfirmBox::confirmed() {
	if (!_confirmed) {
		_confirmed = true;
		if (_confirmedCallback) {
			_confirmedCallback();
		}
	}
}

void ConfirmBox::mouseMoveEvent(QMouseEvent *e) {
	_lastMousePos = e->globalPos();
	updateHover();
}

void ConfirmBox::mousePressEvent(QMouseEvent *e) {
	_lastMousePos = e->globalPos();
	updateHover();
	ClickHandler::pressed();
	return BoxContent::mousePressEvent(e);
}

void ConfirmBox::mouseReleaseEvent(QMouseEvent *e) {
	_lastMousePos = e->globalPos();
	updateHover();
	if (ClickHandlerPtr activated = ClickHandler::unpressed()) {
		Ui::hideLayer();
		App::activateClickHandler(activated, e->button());
	}
	return BoxContent::mouseReleaseEvent(e);
}

void ConfirmBox::leaveEvent(QEvent *e) {
	ClickHandler::clearActive(this);
}

void ConfirmBox::clickHandlerActiveChanged(const ClickHandlerPtr &p, bool active) {
	setCursor(active ? style::cur_pointer : style::cur_default);
	update();
}

void ConfirmBox::clickHandlerPressedChanged(const ClickHandlerPtr &p, bool pressed) {
	update();
}

void ConfirmBox::updateLink() {
	_lastMousePos = QCursor::pos();
	updateHover();
}

void ConfirmBox::updateHover() {
	QPoint m(mapFromGlobal(_lastMousePos));

	auto state = _text.getStateLeft(m.x() - st::boxPadding.left(), m.y() - st::boxPadding.top(), _textWidth, width());

	ClickHandler::setActive(state.link, this);
}

void ConfirmBox::keyPressEvent(QKeyEvent *e) {
	if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
		confirmed();
	} else {
		BoxContent::keyPressEvent(e);
	}
}

void ConfirmBox::paintEvent(QPaintEvent *e) {
	BoxContent::paintEvent(e);

	Painter p(this);

	// draw box title / text
	p.setPen(st::boxTextFg);
	_text.drawLeftElided(p, st::boxPadding.left(), st::boxPadding.top(), _textWidth, width(), 16, style::al_left);
}

InformBox::InformBox(QWidget*, const QString &text, base::lambda_copy<void()> &&closedCallback) : ConfirmBox(ConfirmBox::InformBoxTag(), text, lang(lng_box_ok), std_::move(closedCallback)) {
}

InformBox::InformBox(QWidget*, const QString &text, const QString &doneText, base::lambda_copy<void()> &&closedCallback) : ConfirmBox(ConfirmBox::InformBoxTag(), text, doneText, std_::move(closedCallback)) {
}

MaxInviteBox::MaxInviteBox(QWidget*, const QString &link)
: _text(st::boxTextStyle, lng_participant_invite_sorry(lt_count, Global::ChatSizeMax()), _confirmBoxTextOptions, st::boxWidth - st::boxPadding.left() - st::boxButtonPadding.right())
, _link(link) {
}

void MaxInviteBox::prepare() {
	setMouseTracking(true);

	addButton(lang(lng_box_ok), [this] { closeBox(); });

	_textWidth = st::boxWidth - st::boxPadding.left() - st::boxButtonPadding.right();
	_textHeight = qMin(_text.countHeight(_textWidth), 16 * int(st::boxTextStyle.lineHeight));
	setDimensions(st::boxWidth, st::boxPadding.top() + _textHeight + st::boxTextFont->height + st::boxTextFont->height * 2 + st::newGroupLinkPadding.bottom());
}

void MaxInviteBox::mouseMoveEvent(QMouseEvent *e) {
	updateSelected(e->globalPos());
}

void MaxInviteBox::mousePressEvent(QMouseEvent *e) {
	mouseMoveEvent(e);
	if (_linkOver) {
		Application::clipboard()->setText(_link);

		Ui::Toast::Config toast;
		toast.text = lang(lng_create_channel_link_copied);
		Ui::Toast::Show(App::wnd(), toast);
	}
}

void MaxInviteBox::leaveEvent(QEvent *e) {
	updateSelected(QCursor::pos());
}

void MaxInviteBox::updateSelected(const QPoint &cursorGlobalPosition) {
	QPoint p(mapFromGlobal(cursorGlobalPosition));

	bool linkOver = _invitationLink.contains(p);
	if (linkOver != _linkOver) {
		_linkOver = linkOver;
		update();
		setCursor(_linkOver ? style::cur_pointer : style::cur_default);
	}
}

void MaxInviteBox::paintEvent(QPaintEvent *e) {
	BoxContent::paintEvent(e);

	Painter p(this);

	// draw box title / text
	p.setPen(st::boxTextFg);
	_text.drawLeftElided(p, st::boxPadding.left(), st::boxPadding.top(), _textWidth, width(), 16, style::al_left);

	QTextOption option(style::al_left);
	option.setWrapMode(QTextOption::WrapAnywhere);
	p.setFont(_linkOver ? st::defaultInputField.font->underline() : st::defaultInputField.font);
	p.setPen(st::defaultLinkButton.color);
	p.drawText(_invitationLink, _link, option);
}

void MaxInviteBox::resizeEvent(QResizeEvent *e) {
	BoxContent::resizeEvent(e);
	_invitationLink = myrtlrect(st::boxPadding.left(), st::boxPadding.top() + _textHeight + st::boxTextFont->height, width() - st::boxPadding.left() - st::boxPadding.right(), 2 * st::boxTextFont->height);
}

ConvertToSupergroupBox::ConvertToSupergroupBox(QWidget*, ChatData *chat)
: _chat(chat)
, _text(100)
, _note(100) {
}

void ConvertToSupergroupBox::prepare() {
	QStringList text;
	text.push_back(lang(lng_profile_convert_feature1));
	text.push_back(lang(lng_profile_convert_feature2));
	text.push_back(lang(lng_profile_convert_feature3));
	text.push_back(lang(lng_profile_convert_feature4));

	setTitle(lang(lng_profile_convert_title));

	addButton(lang(lng_profile_convert_confirm), [this] { convertToSupergroup(); });
	addButton(lang(lng_cancel), [this] { closeBox(); });

	_text.setText(st::boxTextStyle, text.join('\n'), _confirmBoxTextOptions);
	_note.setText(st::boxTextStyle, lng_profile_convert_warning(lt_bold_start, textcmdStartSemibold(), lt_bold_end, textcmdStopSemibold()), _confirmBoxTextOptions);
	_textWidth = st::boxWideWidth - st::boxPadding.left() - st::boxButtonPadding.right();
	_textHeight = _text.countHeight(_textWidth);
	setDimensions(st::boxWideWidth, _textHeight + st::boxPadding.bottom() + _note.countHeight(_textWidth));
}

void ConvertToSupergroupBox::convertToSupergroup() {
	MTP::send(MTPmessages_MigrateChat(_chat->inputChat), rpcDone(&ConvertToSupergroupBox::convertDone), rpcFail(&ConvertToSupergroupBox::convertFail));
}

void ConvertToSupergroupBox::convertDone(const MTPUpdates &updates) {
	Ui::hideLayer();
	App::main()->sentUpdatesReceived(updates);
	const QVector<MTPChat> *v = 0;
	switch (updates.type()) {
	case mtpc_updates: v = &updates.c_updates().vchats.c_vector().v; break;
	case mtpc_updatesCombined: v = &updates.c_updatesCombined().vchats.c_vector().v; break;
	default: LOG(("API Error: unexpected update cons %1 (ConvertToSupergroupBox::convertDone)").arg(updates.type())); break;
	}

	PeerData *peer = 0;
	if (v && !v->isEmpty()) {
		for (int32 i = 0, l = v->size(); i < l; ++i) {
			if (v->at(i).type() == mtpc_channel) {
				peer = App::channel(v->at(i).c_channel().vid.v);
				Ui::showPeerHistory(peer, ShowAtUnreadMsgId);
				QTimer::singleShot(ReloadChannelMembersTimeout, App::api(), SLOT(delayedRequestParticipantsCount()));
			}
		}
	}
	if (!peer) {
		LOG(("API Error: channel not found in updates (ProfileInner::migrateDone)"));
	}
}

bool ConvertToSupergroupBox::convertFail(const RPCError &error) {
	if (MTP::isDefaultHandledError(error)) return false;
	Ui::hideLayer();
	return true;
}

void ConvertToSupergroupBox::keyPressEvent(QKeyEvent *e) {
	if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
		convertToSupergroup();
	} else {
		BoxContent::keyPressEvent(e);
	}
}

void ConvertToSupergroupBox::paintEvent(QPaintEvent *e) {
	BoxContent::paintEvent(e);

	Painter p(this);

	// draw box title / text
	p.setPen(st::boxTextFg);
	_text.drawLeft(p, st::boxPadding.left(), 0, _textWidth, width());
	_note.drawLeft(p, st::boxPadding.left(), _textHeight + st::boxPadding.bottom(), _textWidth, width());
}

PinMessageBox::PinMessageBox(QWidget*, ChannelData *channel, MsgId msgId)
: _channel(channel)
, _msgId(msgId)
, _text(this, lang(lng_pinned_pin_sure), Ui::FlatLabel::InitType::Simple, st::boxLabel)
, _notify(this, lang(lng_pinned_notify), true, st::defaultBoxCheckbox) {
}

void PinMessageBox::prepare() {
	_text->resizeToWidth(st::boxWidth - st::boxPadding.left() - st::boxButtonPadding.right());

	addButton(lang(lng_pinned_pin), [this] { pinMessage(); });
	addButton(lang(lng_cancel), [this] { closeBox(); });

	setDimensions(st::boxWidth, st::boxPadding.top() + _text->height() + st::boxMediumSkip + _notify->heightNoMargins() + st::boxPadding.bottom());
}

void PinMessageBox::resizeEvent(QResizeEvent *e) {
	BoxContent::resizeEvent(e);
	_text->moveToLeft(st::boxPadding.left(), st::boxPadding.top());
	_notify->moveToLeft(st::boxPadding.left(), _text->y() + _text->height() + st::boxMediumSkip);
}

void PinMessageBox::pinMessage() {
	if (_requestId) return;

	MTPchannels_UpdatePinnedMessage::Flags flags = 0;
	if (!_notify->checked()) {
		flags |= MTPchannels_UpdatePinnedMessage::Flag::f_silent;
	}
	_requestId = MTP::send(MTPchannels_UpdatePinnedMessage(MTP_flags(flags), _channel->inputChannel, MTP_int(_msgId)), rpcDone(&PinMessageBox::pinDone), rpcFail(&PinMessageBox::pinFail));
}

void PinMessageBox::pinDone(const MTPUpdates &updates) {
	if (App::main()) {
		App::main()->sentUpdatesReceived(updates);
	}
	Ui::hideLayer();
}

bool PinMessageBox::pinFail(const RPCError &error) {
	if (MTP::isDefaultHandledError(error)) return false;
	Ui::hideLayer();
	return true;
}

RichDeleteMessageBox::RichDeleteMessageBox(QWidget*, ChannelData *channel, UserData *from, MsgId msgId)
: _channel(channel)
, _from(from)
, _msgId(msgId)
, _text(this, lang(lng_selected_delete_sure_this), Ui::FlatLabel::InitType::Simple, st::boxLabel)
, _banUser(this, lang(lng_ban_user), false, st::defaultBoxCheckbox)
, _reportSpam(this, lang(lng_report_spam), false, st::defaultBoxCheckbox)
, _deleteAll(this, lang(lng_delete_all_from), false, st::defaultBoxCheckbox) {
}

void RichDeleteMessageBox::prepare() {
	t_assert(_channel != nullptr);

	addButton(lang(lng_box_delete), [this] { deleteAndClear(); });
	addButton(lang(lng_cancel), [this] { closeBox(); });

	_text->resizeToWidth(st::boxWidth - st::boxPadding.left() - st::boxButtonPadding.right());
	setDimensions(st::boxWidth, st::boxPadding.top() + _text->height() + st::boxMediumSkip + _banUser->heightNoMargins() + st::boxLittleSkip + _reportSpam->heightNoMargins() + st::boxLittleSkip + _deleteAll->heightNoMargins() + st::boxPadding.bottom());
}

void RichDeleteMessageBox::resizeEvent(QResizeEvent *e) {
	BoxContent::resizeEvent(e);
	_text->moveToLeft(st::boxPadding.left(), st::boxPadding.top());
	_banUser->moveToLeft(st::boxPadding.left(), _text->bottomNoMargins() + st::boxMediumSkip);
	_reportSpam->moveToLeft(st::boxPadding.left(), _banUser->bottomNoMargins() + st::boxLittleSkip);
	_deleteAll->moveToLeft(st::boxPadding.left(), _reportSpam->bottomNoMargins() + st::boxLittleSkip);
}

void RichDeleteMessageBox::deleteAndClear() {
	if (_banUser->checked()) {
		MTP::send(MTPchannels_KickFromChannel(_channel->inputChannel, _from->inputUser, MTP_boolTrue()), App::main()->rpcDone(&MainWidget::sentUpdatesReceived));
	}
	if (_reportSpam->checked()) {
		MTP::send(MTPchannels_ReportSpam(_channel->inputChannel, _from->inputUser, MTP_vector<MTPint>(1, MTP_int(_msgId))));
	}
	if (_deleteAll->checked()) {
		App::main()->deleteAllFromUser(_channel, _from);
	}
	if (auto item = App::histItemById(_channel ? peerToChannel(_channel->id) : 0, _msgId)) {
		auto wasLast = (item->history()->lastMsg == item);
		item->destroy();

		if (_msgId > 0) {
			auto forEveryone = true;
			App::main()->deleteMessages(_channel, QVector<MTPint>(1, MTP_int(_msgId)), forEveryone);
		} else if (wasLast) {
			App::main()->checkPeerHistory(_channel);
		}
	}
	Ui::hideLayer();
}

ConfirmInviteBox::ConfirmInviteBox(QWidget*, const QString &title, const MTPChatPhoto &photo, int count, const QVector<UserData*> &participants)
: _title(this, st::confirmInviteTitle)
, _status(this, st::confirmInviteStatus)
, _participants(participants) {
	_title->setText(title);
	QString status;
	if (_participants.isEmpty() || _participants.size() >= count) {
		status = lng_chat_status_members(lt_count, count);
	} else {
		status = lng_group_invite_members(lt_count, count);
	}
	_status->setText(status);
	if (photo.type() == mtpc_chatPhoto) {
		auto &d = photo.c_chatPhoto();
		auto location = App::imageLocation(160, 160, d.vphoto_small);
		if (!location.isNull()) {
			_photo = ImagePtr(location);
			if (!_photo->loaded()) {
				subscribe(FileDownload::ImageLoaded(), [this] { update(); });
				_photo->load();
			}
		}
	}
	if (!_photo) {
		_photoEmpty.set(0, title);
	}
}

void ConfirmInviteBox::prepare() {
	addButton(lang(lng_group_invite_join), [this] {
		if (auto main = App::main()) {
			main->onInviteImport();
		}
	});
	addButton(lang(lng_cancel), [this] { closeBox(); });

	if (_participants.size() > 4) {
		_participants.resize(4);
	}

	auto newHeight = st::confirmInviteStatusTop + _status->height() + st::boxPadding.bottom();
	if (!_participants.isEmpty()) {
		int skip = (st::boxWideWidth - 4 * st::confirmInviteUserPhotoSize) / 5;
		int padding = skip / 2;
		_userWidth = (st::confirmInviteUserPhotoSize + 2 * padding);
		int sumWidth = _participants.size() * _userWidth;
		int left = (st::boxWideWidth - sumWidth) / 2;
		for_const (auto user, _participants) {
			auto name = new Ui::FlatLabel(this, st::confirmInviteUserName);
			name->resizeToWidth(st::confirmInviteUserPhotoSize + padding);
			name->setText(user->firstName.isEmpty() ? App::peerName(user) : user->firstName);
			name->moveToLeft(left + (padding / 2), st::confirmInviteUserNameTop);
			left += _userWidth;
		}

		newHeight += st::confirmInviteUserHeight;
	}
	setDimensions(st::boxWideWidth, newHeight);
}

void ConfirmInviteBox::resizeEvent(QResizeEvent *e) {
	BoxContent::resizeEvent(e);
	_title->move((width() - _title->width()) / 2, st::confirmInviteTitleTop);
	_status->move((width() - _status->width()) / 2, st::confirmInviteStatusTop);
}

void ConfirmInviteBox::paintEvent(QPaintEvent *e) {
	BoxContent::paintEvent(e);

	Painter p(this);

	if (_photo) {
		p.drawPixmap((width() - st::confirmInvitePhotoSize) / 2, st::confirmInvitePhotoTop, _photo->pixCircled(st::confirmInvitePhotoSize, st::confirmInvitePhotoSize));
	} else {
		_photoEmpty.paint(p, (width() - st::confirmInvitePhotoSize) / 2, st::confirmInvitePhotoTop, width(), st::confirmInvitePhotoSize);
	}

	int sumWidth = _participants.size() * _userWidth;
	int left = (width() - sumWidth) / 2;
	for_const (auto user, _participants) {
		user->paintUserpicLeft(p, left + (_userWidth - st::confirmInviteUserPhotoSize) / 2, st::confirmInviteUserPhotoTop, width(), st::confirmInviteUserPhotoSize);
		left += _userWidth;
	}
}

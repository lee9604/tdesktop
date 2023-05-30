/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace Data {
struct StoriesList;
struct FileOrigin;
} // namespace Data

namespace Media::Player {
struct TrackState;
} // namespace Media::Player

namespace Media::Stories {

class Delegate;
class Controller;

struct ContentLayout {
	QRect geometry;
	float64 fade = 0.;
	int radius = 0;
	bool headerOutside = false;
};

enum class SiblingType;

struct SiblingView {
	QImage image;
	ContentLayout layout;
	QImage userpic;
	QPoint userpicPosition;
	QImage name;
	QPoint namePosition;
	float64 nameOpacity = 0.;

	[[nodiscard]] bool valid() const {
		return !image.isNull();
	}
	explicit operator bool() const {
		return valid();
	}
};

class View final {
public:
	explicit View(not_null<Delegate*> delegate);
	~View();

	void show(
		const std::vector<Data::StoriesList> &lists,
		int index,
		int subindex);
	void ready();

	[[nodiscard]] bool canDownload() const;
	[[nodiscard]] QRect finalShownGeometry() const;
	[[nodiscard]] rpl::producer<QRect> finalShownGeometryValue() const;
	[[nodiscard]] ContentLayout contentLayout() const;
	[[nodiscard]] SiblingView sibling(SiblingType type) const;
	[[nodiscard]] Data::FileOrigin fileOrigin() const;
	[[nodiscard]] TextWithEntities captionText() const;
	void showFullCaption();

	void updatePlayback(const Player::TrackState &state);

	[[nodiscard]] bool subjumpAvailable(int delta) const;
	[[nodiscard]] bool subjumpFor(int delta) const;
	[[nodiscard]] bool jumpFor(int delta) const;

	[[nodiscard]] bool paused() const;
	void togglePaused(bool paused);

	[[nodiscard]] rpl::lifetime &lifetime();

private:
	const std::unique_ptr<Controller> _controller;

};

} // namespace Media::Stories
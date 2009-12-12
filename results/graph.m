width = 640 / 72;
height = 480 / 72;

% load the data
data

% plot data for source_node1
plot(source_node1(:,1), source_node1(:,3), '.');
set(gca, 'FontName', 'Courier', 'Fontsize', 12);
set(title('Measurements Received from Sender -- (node1 to node2)'), ...
'FontSize', 16, 'FontName', 'Helvetica');
set(xlabel('Time (s)'), 'FontSize', 16, 'FontName', 'Helvetica');
set(ylabel('Max Sequence Number of Sent Packets'), 'FontSize', 16, 'FontName', ...
'Helvetica');
grid on;

% save to eps
set(gcf, 'PaperPositionMode', 'manual');
set(gcf, 'PaperUnits', 'inches');
set(gcf, 'PaperPosition', [0 0 width height]);

print -dpng source_node1.png

% plot data for source_node2
plot(source_node2(:,1), source_node2(:,3), '.');
set(gca, 'FontName', 'Courier', 'Fontsize', 12);
set(title('Measurements Received from Sender -- (node2 to node1)'), ...
'FontSize', 16, 'FontName', 'Helvetica');
set(xlabel('Time (s)'), 'FontSize', 16, 'FontName', 'Helvetica');
set(ylabel('Max Sequence Number of Sent Packets'), 'FontSize', 16, 'FontName', ...
'Helvetica');
grid on;

% save to eps
set(gcf, 'PaperPositionMode', 'manual');
set(gcf, 'PaperUnits', 'inches');
set(gcf, 'PaperPosition', [0 0 width height]);

print -dpng source_node2.png

% plot data for sink_node1
plot(sink_node1(:,1), sink_node1(:,3), '.');
set(gca, 'FontName', 'Courier', 'Fontsize', 12);
set(title('Measurements Received from Receiver -- (node2 to node1)'), ...
'FontSize', 16, 'FontName', 'Helvetica');
set(xlabel('Time (s)'), 'FontSize', 16, 'FontName', 'Helvetica');
set(ylabel('Max Sequence Number of Received Packets'), 'FontSize', 16, 'FontName', ...
'Helvetica');
grid on;

% save to eps
set(gcf, 'PaperPositionMode', 'manual');
set(gcf, 'PaperUnits', 'inches');
set(gcf, 'PaperPosition', [0 0 width height]);

print -dpng sink_node1.png

% plot data for sink_node2
plot(sink_node2(:,1), sink_node2(:,3), '.');
set(gca, 'FontName', 'Courier', 'Fontsize', 12);
set(title('Measurements Received from Receiver -- (node1 to node2)'), ...
'FontSize', 16, 'FontName', 'Helvetica');
set(xlabel('Time (s)'), 'FontSize', 16, 'FontName', 'Helvetica');
set(ylabel('Max Sequence Number of Received Packets'), 'FontSize', 16, 'FontName', ...
'Helvetica');
grid on;

% save to eps
set(gcf, 'PaperPositionMode', 'manual');
set(gcf, 'PaperUnits', 'inches');
set(gcf, 'PaperPosition', [0 0 width height]);

print -dpng sink_node2.png

% plot all data on same plot
plot(source_node1(:,1), source_node1(:,3), '.b', source_node2(:,1), source_node2(:,3), '.g',...
sink_node1(:,1), sink_node1(:,3), '.r', sink_node2(:,1), sink_node2(:,3), '.m');
set(gca, 'FontName', 'Courier', 'Fontsize', 12);
set(title('Measurements Received from Sender/Receiver -- (combined flows)'), ...
'FontSize', 16, 'FontName', 'Helvetica');
set(xlabel('Time (s)'), 'FontSize', 16, 'FontName', 'Helvetica');
set(ylabel('Max Sequence Number of Packets Seen'), 'FontSize', 16, 'FontName', ...
'Helvetica');
set(legend('sender -- node1', 'sender -- node2', 'receiver -- node1', 'receiver -- node2'), ...
'Fontsize', 14, 'Fontname', 'Helvetica', 'Location', 'NorthWest')
grid on;

% save to eps
set(gcf, 'PaperPositionMode', 'manual');
set(gcf, 'PaperUnits', 'inches');
set(gcf, 'PaperPosition', [0 0 width height]);

print -dpng combined.png

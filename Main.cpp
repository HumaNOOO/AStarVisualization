#include <array>
#include <limits>
#include <cmath>
#include <ranges>
#include <format>
#include <algorithm>
#include <stack>
#include <sstream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <iostream>
#include <random>

#include <SFML/Graphics.hpp>

std::atomic exit_program = false;
std::atomic stop_a_star = false;
int map_width = 50;
int map_height = 50;
struct Node;
sf::Vector2f p1{ -1,-1 };
sf::Vector2f p2{ -1,-1 };
float m = 0;
float b = 0;
Node* start;
Node* stop;
std::vector<Node*> stops;

void listener()
{
	while (!exit_program)
	{
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) exit_program = true;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) stop_a_star = true;
	}
}

inline void reset_line()
{
	p1.x = -1;
	p1.y = -1;
	p2.x = -1;
	p2.y = -1;
}

struct Node
{
	Node()
	{
		gscore = std::numeric_limits<float>::max();
		fscore = std::numeric_limits<float>::max();
		parent = nullptr;
		blocking = false;
		visited = false;
		isPath = false;
		inOpenSet = false;
		isTempPath = false;
	}

	float gscore;
	float fscore;
	Node* parent;
	std::vector<Node*> neighbors;
	float x;
	float y;
	int gridx;
	int gridy;
	inline static float sizex = 20;
	inline static float sizey = 20;
	inline static float spacing = 1;
	bool blocking;
	bool visited;
	bool isPath;
	bool inOpenSet;
	bool isTempPath;
};

using Map = std::vector<std::vector<Node>>;
class BoardDrawer : public sf::Transformable, public sf::Drawable
{
public:
	BoardDrawer(Map& map) : ptr{ map }
	{
		vertices.setPrimitiveType(sf::Quads);
		line_vertices.setPrimitiveType(sf::Quads);

		for (int i = 0; i < map_height; i++)
		{
			for (int j = 0; j < map_width; j++)
			{
				vertices.append(sf::Vertex({ i * (Node::sizex + Node::spacing), j * (Node::sizey + Node::spacing) }, sf::Color::White));
				vertices.append(sf::Vertex({ i * (Node::sizex + Node::spacing) + Node::sizex, j * (Node::sizey + Node::spacing) }, sf::Color::White));
				vertices.append(sf::Vertex({ i * (Node::sizex + Node::spacing) + Node::sizex, j * (Node::sizey + Node::spacing) + Node::sizey }, sf::Color::White));
				vertices.append(sf::Vertex({ i * (Node::sizex + Node::spacing), j * (Node::sizey + Node::spacing) + Node::sizey }, sf::Color::White));
			}
		}
	}

	void reset_board()
	{
		vertices.clear();
		line_vertices.clear();

		for (int i = 0; i < map_height; i++)
		{
			for (int j = 0; j < map_width; j++)
			{
				vertices.append(sf::Vertex({ i * (Node::sizex + Node::spacing), j * (Node::sizey + Node::spacing) }, sf::Color::White));
				vertices.append(sf::Vertex({ i * (Node::sizex + Node::spacing) + Node::sizex, j * (Node::sizey + Node::spacing) }, sf::Color::White));
				vertices.append(sf::Vertex({ i * (Node::sizex + Node::spacing) + Node::sizex, j * (Node::sizey + Node::spacing) + Node::sizey }, sf::Color::White));
				vertices.append(sf::Vertex({ i * (Node::sizex + Node::spacing), j * (Node::sizey + Node::spacing) + Node::sizey }, sf::Color::White));
			}
		}
	}

	void selective_update(const std::vector<Node*>& changed_nodes)
	{
		for (Node* node : changed_nodes)
		{
			const int offset = (node->gridx * map_width + node->gridy) * 4;
			if (node->inOpenSet)
			{
				vertices[offset].color = sf::Color::Green;
				vertices[offset + 1].color = sf::Color::Green;
				vertices[offset + 2].color = sf::Color::Green;
				vertices[offset + 3].color = sf::Color::Green;
				node->isPath = false;
				node->inOpenSet = false;
			}
			else if (node->visited)
			{
				vertices[offset].color = sf::Color(0, 255, 0, 80);
				vertices[offset + 1].color = sf::Color(0, 255, 0, 80);
				vertices[offset + 2].color = sf::Color(0, 255, 0, 80);
				vertices[offset + 3].color = sf::Color(0, 255, 0, 80);
			}
			else
			{
				vertices[offset].color = sf::Color::White;
				vertices[offset + 1].color = sf::Color::White;
				vertices[offset + 2].color = sf::Color::White;
				vertices[offset + 3].color = sf::Color::White;
			}
		}
	}

	void update_maze_selected(const int y, const int x)
	{
		const int offset = (y * map_width + x) * 4;
		vertices[offset].color = sf::Color::White;
		vertices[offset + 1].color = sf::Color::White;
		vertices[offset + 2].color = sf::Color::White;
		vertices[offset + 3].color = sf::Color::White;
		ptr[y][x].blocking = false;
	}

	void update()
	{
		for (int i = 0; i < map_height; i++)
		{
			for (int j = 0; j < map_width; j++)
			{
				const int offset = (i * map_width + j) * 4;
				if (ptr[i][j].blocking)
				{
					vertices[offset].color = sf::Color(0, 0, 0, 100);
					vertices[offset + 1].color = sf::Color(0, 0, 0, 100);
					vertices[offset + 2].color = sf::Color(0, 0, 0, 100);
					vertices[offset + 3].color = sf::Color(0, 0, 0, 100);
				}
				else if (ptr[i][j].isPath)
				{
					vertices[offset].color = sf::Color(78, 125, 212);
					vertices[offset + 1].color = sf::Color(78, 125, 212);
					vertices[offset + 2].color = sf::Color(78, 125, 212);
					vertices[offset + 3].color = sf::Color(78, 125, 212);
				}
				else if (ptr[i][j].isTempPath)
				{
					vertices[offset].color = sf::Color(255, 255, 255, 150);
					vertices[offset + 1].color = sf::Color(255, 255, 255, 150);
					vertices[offset + 2].color = sf::Color(255, 255, 255, 150);
					vertices[offset + 3].color = sf::Color(255, 255, 255, 150);
					ptr[i][j].isTempPath = false;
				}
				else if (ptr[i][j].inOpenSet)
				{
					vertices[offset].color = sf::Color::Green;
					vertices[offset + 1].color = sf::Color::Green;
					vertices[offset + 2].color = sf::Color::Green;
					vertices[offset + 3].color = sf::Color::Green;
				}
				else if (ptr[i][j].visited)
				{
					vertices[offset].color = sf::Color(0, 255, 0, 80);
					vertices[offset + 1].color = sf::Color(0, 255, 0, 80);
					vertices[offset + 2].color = sf::Color(0, 255, 0, 80);
					vertices[offset + 3].color = sf::Color(0, 255, 0, 80);
				}
				else
				{
					vertices[offset].color = sf::Color::White;
					vertices[offset + 1].color = sf::Color::White;
					vertices[offset + 2].color = sf::Color::White;
					vertices[offset + 3].color = sf::Color::White;
				}
			}
		}

		const int offset_end = (stop->gridx * map_width + stop->gridy) * 4;
		vertices[offset_end].color = sf::Color::Red;
		vertices[offset_end + 1].color = sf::Color::Red;
		vertices[offset_end + 2].color = sf::Color::Red;
		vertices[offset_end + 3].color = sf::Color::Red;

		const int offset_start = (start->gridx * map_width + start->gridy) * 4;
		vertices[offset_start].color = sf::Color::Yellow;
		vertices[offset_start + 1].color = sf::Color::Yellow;
		vertices[offset_start + 2].color = sf::Color::Yellow;
		vertices[offset_start + 3].color = sf::Color::Yellow;

		for (const Node* node : stops)
		{
			const int offset = (node->gridx * map_width + node->gridy) * 4;

			vertices[offset].color = sf::Color::Magenta;
			vertices[offset + 1].color = sf::Color::Magenta;
			vertices[offset + 2].color = sf::Color::Magenta;
			vertices[offset + 3].color = sf::Color::Magenta;
		}
	}

	void append_walls()
	{
		for (int i = 0; i < map_height; i++)
		{
			for (int j = 0; j < map_width; j++)
			{
				if (j + 1 < map_width && std::ranges::find(ptr[j][i].neighbors, &ptr[j + 1][i]) != ptr[j][i].neighbors.end())
				{
					vertices.append(sf::Vertex({ j * (Node::spacing + Node::sizex) + Node::sizex, i * (Node::sizey + Node::spacing) }, sf::Color::White));
					vertices.append(sf::Vertex({ j * (Node::spacing + Node::sizex) + Node::sizex + Node::spacing, i * (Node::sizey + Node::spacing) }, sf::Color::White));
					vertices.append(sf::Vertex({ j * (Node::spacing + Node::sizex) + Node::sizex + Node::spacing, i * (Node::sizey + Node::spacing) + Node::sizey }, sf::Color::White));
					vertices.append(sf::Vertex({ j * (Node::spacing + Node::sizex) + Node::sizex, i * (Node::sizey + Node::spacing) + Node::sizey }, sf::Color::White));
				}

				if (i + 1 < map_height && std::ranges::find(ptr[j][i].neighbors, &ptr[j][i + 1]) != ptr[j][i].neighbors.end())
				{
					vertices.append(sf::Vertex({ j * (Node::spacing + Node::sizex), i * (Node::sizey + Node::spacing) + Node::sizey }, sf::Color::White));
					vertices.append(sf::Vertex({ j * (Node::spacing + Node::sizex) + Node::sizex, i * (Node::sizey + Node::spacing) + Node::sizey }, sf::Color::White));
					vertices.append(sf::Vertex({ j * (Node::spacing + Node::sizex) + Node::sizex, i * (Node::sizey + Node::spacing) + Node::sizey + Node::spacing }, sf::Color::White));
					vertices.append(sf::Vertex({ j * (Node::spacing + Node::sizex), i * (Node::sizey + Node::spacing) + Node::sizey + Node::spacing }, sf::Color::White));
				}
			}
		}
	}

	void append_wall(const Node* left, const Node* right)
	{
		if (left->x < right->x)
		{
			line_vertices.append(sf::Vertex({ left->x + Node::sizex, left->y }, sf::Color::White));
			line_vertices.append(sf::Vertex({ left->x + Node::sizex + Node::spacing, left->y }, sf::Color::White));
			line_vertices.append(sf::Vertex({ left->x + Node::sizex + Node::spacing, left->y + Node::sizey }, sf::Color::White));
			line_vertices.append(sf::Vertex({ left->x + Node::sizex, left->y + Node::sizey }, sf::Color::White));
		}
		else if (left->x > right->x)
		{
			line_vertices.append(sf::Vertex({ right->x + Node::sizex, right->y }, sf::Color::White));
			line_vertices.append(sf::Vertex({ right->x + Node::sizex + Node::spacing, right->y }, sf::Color::White));
			line_vertices.append(sf::Vertex({ right->x + Node::sizex + Node::spacing, right->y + Node::sizey }, sf::Color::White));
			line_vertices.append(sf::Vertex({ right->x + Node::sizex, right->y + Node::sizey }, sf::Color::White));
		}
		else if (left->y < right->y)
		{
			line_vertices.append(sf::Vertex({ left->x, left->y + Node::sizey }, sf::Color::White));
			line_vertices.append(sf::Vertex({ left->x + Node::sizex, left->y + Node::sizey }, sf::Color::White));
			line_vertices.append(sf::Vertex({ left->x + Node::sizex, left->y + Node::sizey + Node::spacing }, sf::Color::White));
			line_vertices.append(sf::Vertex({ left->x, left->y + Node::sizey + Node::spacing }, sf::Color::White));
		}
		else
		{
			line_vertices.append(sf::Vertex({ right->x, right->y + Node::sizey }, sf::Color::White));
			line_vertices.append(sf::Vertex({ right->x + Node::sizex, right->y + Node::sizey }, sf::Color::White));
			line_vertices.append(sf::Vertex({ right->x + Node::sizex, right->y + Node::sizey + Node::spacing }, sf::Color::White));
			line_vertices.append(sf::Vertex({ right->x, right->y + Node::sizey + Node::spacing }, sf::Color::White));
		}
	}

	void reset_line_vertices()
	{
		line_vertices.clear();
	}
private:
	sf::VertexArray vertices;
	sf::VertexArray line_vertices;
	Map& ptr;

	void draw(sf::RenderTarget& rt, sf::RenderStates states) const override
	{
		states.transform *= getTransform();
		rt.draw(vertices, states);
		rt.draw(line_vertices, states);
	}
};
class AStarApp
{
public:
	AStarApp() : window{ sf::VideoMode(2560,1440), "A*", sf::Style::Default }, board(map), total_time(0.f), real_time(0.f), visited_nodes(0.f), path_length(0)
	{
		reset_map();

		font.loadFromFile("mono.ttf");
		text.setFont(font);
		text.setPosition(Node::sizex * map_height + Node::spacing * map_height, 5.f);
		text.setCharacterSize(24);
		window.setPosition({ 0,0 });
	}

	void run()
	{
		ss << std::fixed;
		std::thread worker(&listener);
		sf::Event e{};
		while (!exit_program)
		{
			while (window.pollEvent(e))
			{
				switch (e.type)
				{
				case sf::Event::Closed:
					window.close();
					exit_program = true;
					break;
				case sf::Event::KeyReleased:
					switch (e.key.code)
					{
					case sf::Keyboard::G:
						generate_obstacles();
						break;
					case sf::Keyboard::Right:
						map_height++;
						reset_map();
						break;
					case sf::Keyboard::Left:
						map_height--;
						reset_map();
						break;
					case sf::Keyboard::Up:
						map_width--;
						reset_map();
						break;
					case sf::Keyboard::Down:
						map_width++;
						reset_map();
						break;
					case sf::Keyboard::Equal:
						Node::sizex++;
						Node::sizey++;
						reset_map();
						break;
					case sf::Keyboard::Hyphen:
						Node::sizex--;
						Node::sizey--;
						reset_map();
						break;
					}
					break;
				case sf::Event::Resized:
					window.setView(sf::View(sf::FloatRect(0, 0, e.size.width, e.size.height)));
					break;
				}
			}

			window.setTitle("A*" + std::to_string(1.f / dt.restart().asSeconds()));

			if (const sf::Vector2i mp = sf::Mouse::getPosition(window); !(mp.x < 0 || mp.x >= map_height * (Node::sizex + Node::spacing) || mp.y < 0 || mp.y >= map_width * (Node::sizey + Node::spacing)))
			{
				const int x = static_cast<int>(mp.x / (Node::sizex + Node::spacing));
				const int y = static_cast<int>(mp.y / (Node::sizey + Node::spacing));

				if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt))
				{
					if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
					{
						if (std::ranges::find(stops, &map[x][y]) == stops.end()) stops.push_back(&map[x][y]);
					}
					else if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
					{
						std::erase_if(stops, [&](const Node* node) { return node == &map[x][y]; });
					}
				}
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
				{
					if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
					{
						start = &map[x][y];
					}
					else if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
					{
						stop = &map[x][y];
					}
				}
				else if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
				{
					if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
					{
						p2 = sf::Vector2f{ -1,-1 };
						p1 = sf::Vector2f(mp);
						if (p2.x - p1.x != 0) m = (p2.y - p1.y) / (p2.x - p1.x);
						b = -(m * p1.x - p1.y);
					}
					else if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
					{
						if (p1.x != -1 && p1.y != -1)
						{
							p2 = sf::Vector2f(mp);
							if (p2.x - p1.x != 0) m = (p2.y - p1.y) / (p2.x - p1.x);
							b = -(m * p1.x - p1.y);
						}
					}
				}
				else if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
				{
					map[x][y].blocking = true;
				}
				else if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
				{
					map[x][y].blocking = false;
				}
			}

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
			{
				visited_nodes = 0;
				path_length = 0;
				for (auto& row : map)
				{
					for (Node& node : row)
					{
						node.parent = nullptr;
						node.inOpenSet = false;
						node.visited = false;
						node.isPath = false;
						node.gscore = std::numeric_limits<float>::max();
						node.fscore = std::numeric_limits<float>::max();

						reset_line();

						total_time = 0.f;
						real_time = 0.f;
					}
				}
				stop_a_star = false;
				reset_line();
				board.update();
				animate_a_star();
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::R))
			{
				stops.clear();
				visited_nodes = 0;
				path_length = 0;
				for (auto& row : map)
				{
					for (Node& node : row)
					{
						node.blocking = false;
						node.parent = nullptr;
						node.inOpenSet = false;
						node.visited = false;
						node.isPath = false;
						node.gscore = std::numeric_limits<float>::max();
						node.fscore = std::numeric_limits<float>::max();

						reset_line();

						total_time = 0.f;
						real_time = 0.f;
					}
				}
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::C))
			{
				reset_line();
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::B))
			{
				build_maze();
			}

			ss << "Total time: " << total_time << "s\nReal time(no visualization): " << real_time << "s\nVisited nodes: " << static_cast<int>(visited_nodes) << '/' << static_cast<int>(map_width * map_height) << " (" << visited_nodes / (map_width * map_height) * 100.f << "%)\nPath length: " << path_length;
			text.setString(ss.str());
			ss.str("");
			draw_line();
			board.update();
			window.clear();
			window.draw(board);
			window.draw(text);
			window.display();
		}
		window.close();
		worker.join();
	}


private:
	void animate_a_star()
	{
		Node* start_cpy = start;
		Node* stop_cpy = stop;
		std::vector<Node*> stops_cpy = stops;
		stops_cpy.insert(stops_cpy.begin(), stop);
		std::ranges::sort(stops_cpy, [&](const Node* left, const Node* right) { return distance(*left, *start) > distance(*right, *start); });
		stop = stops_cpy.back();
		stops_cpy.pop_back();
		visited_nodes = 0.f;
		sf::Clock clk;
		a_star();
		real_time = clk.restart().asSeconds();

		for (auto& row : map)
		{
			for (Node& node : row)
			{
				node.parent = nullptr;
				node.gscore = std::numeric_limits<float>::max();
				node.fscore = std::numeric_limits<float>::max();
				node.isPath = false;
				node.isTempPath = false;
				node.inOpenSet = false;
				node.visited = false;
			}
		}
		clk.restart();

		std::vector<Node*> open_set{ start };
		open_set.push_back(start);
		start->gscore = 0;
		start->fscore = distance(*start, *stop);

		sf::RectangleShape rs;
		rs.setSize({ Node::sizex, Node::sizey });

		std::vector<Node*> changed;
		while (!open_set.empty() && !exit_program)
		{
			if (stop_a_star) return;
			total_time += clk.restart().asSeconds();
			Node* current = open_set.back();

			if (current == stop)
			{
				if (stops_cpy.empty())
				{
					path_length = 0;
					while (current->parent != nullptr)
					{
						path_length++;
						current->isPath = true;
						current = current->parent;
					}
					current->isPath = true;

					for (Node* node : open_set)
					{
						node->inOpenSet = true;
					}
					start = start_cpy;
					stop = stop_cpy;
					return;
				}

				start = stop;
				std::ranges::sort(stops_cpy, [&](const Node* left, const Node* right) { return distance(*left, *start) > distance(*right, *start); });
				stop = stops_cpy.back();
				stops_cpy.pop_back();
				open_set.clear();
				open_set.push_back(start);
			}
			ss << "Total time: " << total_time << "s\nReal time(no visualization): " << real_time << "s\nVisited nodes: " << static_cast<int>(visited_nodes) << '/' << map_width * map_height << " (" << visited_nodes / (map_width * map_height) * 100.f << "%)\nPath length: " << path_length;
			path_length = 0;
			text.setString(ss.str());
			ss.str("");
			window.clear();
			for (Node* node : open_set)
			{
				changed.push_back(node);
				node->inOpenSet = true;
			}
			board.selective_update(changed);
			changed.clear();
			window.draw(board);
			rs.setFillColor(sf::Color(78, 125, 212));
			while (current->parent != nullptr)
			{
				if (current->parent == start) break;
				path_length++;
				rs.setPosition(current->parent->x, current->parent->y);
				window.draw(rs);
				current = current->parent;
			}
			window.draw(text);

			rs.setFillColor(sf::Color::Yellow);
			rs.setPosition(start->x, start->y);
			window.draw(rs);
			window.display();
			//std::this_thread::sleep_for(std::chrono::milliseconds(10));
			current = open_set.back();
			current->visited = true;
			visited_nodes++;
			changed.push_back(current);

			open_set.pop_back();
			for (Node* neighbor : current->neighbors)
			{
				if (neighbor->blocking) continue;
				const float tscore = current->gscore + 1;

				if (!neighbor->visited || tscore < neighbor->gscore)
				{
					neighbor->parent = current;
					neighbor->gscore = tscore;
					neighbor->fscore = tscore + distance(*neighbor, *stop);

					if (!neighbor->visited && std::ranges::find(open_set, neighbor) == open_set.end())
					{
						open_set.push_back(neighbor);
					}
				}
			}
			std::ranges::sort(open_set, [&](const Node* left, const Node* right) { return left->fscore > right->fscore; });
		}
		path_length = 0;
		start = start_cpy;
		stop = stop_cpy;
	}

	void a_star()
	{
		std::vector<Node*> open_set{ start };
		open_set.push_back(start);
		start->gscore = 0;
		start->fscore = distance(*start, *stop);

		std::vector<Node*> changed;
		while (!open_set.empty() && !exit_program)
		{
			Node* current = open_set.back();

			if (current == stop)
			{
				while (current->parent != nullptr)
				{
					current = current->parent;
				}
				return;
			}
			current->visited = true;
			open_set.pop_back();
			for (Node* neighbor : current->neighbors)
			{
				if (neighbor->blocking) continue;
				const float tscore = current->gscore + 1;

				if (!neighbor->visited || tscore < neighbor->gscore)
				{
					neighbor->parent = current;
					neighbor->gscore = tscore;
					neighbor->fscore = tscore + distance(*neighbor, *stop);

					if (!neighbor->visited && std::ranges::find(open_set, neighbor) == open_set.end())
					{
						open_set.push_back(neighbor);
					}
				}
			}
			std::ranges::sort(open_set, [&](const Node* left, const Node* right) { return left->fscore > right->fscore; });
		}
	}

	void draw_path(const std::vector<sf::Vertex>& path)
	{
		window.draw(path.data(), path.size(), sf::Lines);
	}

	void draw_line()
	{
		sf::Vector2f start = p1;
		sf::Vector2f end = p2;

		if (start.x > end.x && (p2.x != -1 && p2.y != -1))
		{
			std::swap(start, end);
		}

		if (end.x != -1 && end.y != -1)
		{
			const float horizontal_distance = std::fabs(end.x - start.x);
			const float vertical_distance = std::fabs(end.y - start.y);

			if (horizontal_distance > vertical_distance)
			{
				const int end_x = static_cast<int>(end.x / (Node::sizex + Node::spacing));
				int x = static_cast<int>(start.x / (Node::sizex + Node::spacing));
				while (x != end_x + 1)
				{
					const int y = static_cast<int>(m * start.x + b) / static_cast<int>(Node::sizex + Node::spacing);

					start.x += Node::sizex + Node::spacing;
					if (x >= map_height || x < 0 || y >= map_width || y < 0) break;
					map[x][y].blocking = true;
					x++;
				}
			}
			else
			{
				if (start.y > end.y)
				{
					std::swap(start, end);
				}

				const int end_y = static_cast<int>(end.y / (Node::sizey + Node::spacing));
				int y = static_cast<int>(start.y / (Node::sizey + Node::spacing));
				while (y != end_y + 1)
				{
					int x = static_cast<int>((start.y - b) / m) / static_cast<int>(Node::sizey + Node::spacing);

					start.y += Node::sizey + Node::spacing;
					if (x >= map_height || x < 0 || y >= map_width || y < 0) break;
					map[x][y].blocking = true;
					y++;
				}
			}

			p1 = p2;
			p2 = sf::Vector2f(-1, -1);
		}
		else
		{
			end = sf::Vector2f(sf::Mouse::getPosition(window));

			if (start.x > end.x)
			{
				std::swap(start, end);
			}

			const float horizontal_distance = std::fabs(end.x - start.x);
			const float vertical_distance = std::fabs(end.y - start.y);

			if (end == start || p1.x == -1 && p1.y == -1) return;
			m = (end.y - start.y) / (end.x - start.x);
			b = -(m * start.x - start.y);

			if (horizontal_distance > vertical_distance)
			{
				const int end_x = static_cast<int>(end.x / (Node::sizex + Node::spacing));
				int x = static_cast<int>(start.x / (Node::sizex + Node::spacing));
				while (x <= end_x)
				{
					const int y = (m * start.x + b) / (Node::sizex + Node::spacing);

					start.x += Node::sizex + Node::spacing;
					if (x >= map_height || x < 0 || y >= map_width || y < 0) break;
					map[x][y].isTempPath = true;
					x++;
				}
			}
			else
			{
				if(start.y > end.y)
				{
					std::swap(start, end);

					m = (end.y - start.y) / (end.x - start.x);
					b = -(m * start.x - start.y);
				}

				const int end_y = static_cast<int>(end.y / (Node::sizey + Node::spacing));
				int y = static_cast<int>(start.y / (Node::sizey + Node::spacing));
				while (y <= end_y)
				{
					int x = ((start.y - b) / m) / (Node::sizey + Node::spacing);

					start.y += Node::sizey + Node::spacing;
					if (x >= map_height || x < 0 || y >= map_width || y < 0) break;
					map[x][y].isTempPath = true;
					y++;
				}
			}
		}
	}

	bool push_if_valid(std::stack<Node*>& nodes, Node* const current, const int x, const int y)
	{
		if (!(x < 0 || x >= map_height || y < 0 || y >= map_width) && !map[x][y].visited)
		{
			map[x][y].neighbors.push_back(current);
			current->neighbors.push_back(&map[x][y]);
			visited_nodes++;
			nodes.push(&map[x][y]);
			return true;
		}
		return false;
	}

	void build_maze()
	{
		ss.unsetf(std::ios_base::fixed);
		std::random_device rd;
		std::mt19937_64 twister(rd());
		std::uniform_int_distribution<int> distribution(0, 3);
		std::stack<Node*> nodes;
		nodes.push(&map[0][0]);
		board.reset_line_vertices();

		for (auto& row : map)
		{
			for (Node& node : row)
			{
				node.neighbors.clear();
				node.blocking = true;
			}
		}
		board.update();

		enum class Direction
		{
			North,
			South,
			West,
			East
		};

		sf::RectangleShape rs;
		rs.setSize({ Node::sizex, Node::sizey });
		rs.setFillColor(sf::Color::Red);

		while (!nodes.empty())
		{
			Node* current = nodes.top();
			current->visited = true;
			board.update_maze_selected(current->gridx, current->gridy);

			if (!((current->gridy - 1 >= 0 && !map[current->gridx][current->gridy - 1].visited) ||
				  (current->gridy + 1 < map_width && !map[current->gridx][current->gridy + 1].visited) ||
				  (current->gridx - 1 >= 0 && !map[current->gridx - 1][current->gridy].visited) ||
				  (current->gridx + 1 < map_height && !map[current->gridx + 1][current->gridy].visited)))
			{
				nodes.pop();
				continue;
			}

			bool passed = false;

			while (!passed)
			{
				passed = false;
				const auto direction = static_cast<Direction>(distribution(twister));

				if      (direction == Direction::North) passed = push_if_valid(nodes, current, current->gridx, current->gridy - 1);
				else if (direction == Direction::South) passed = push_if_valid(nodes, current, current->gridx, current->gridy + 1);
				else if (direction == Direction::West)  passed = push_if_valid(nodes, current, current->gridx - 1, current->gridy);
				else if (direction == Direction::East)  passed = push_if_valid(nodes, current, current->gridx + 1, current->gridy);
			}
			board.append_wall(current, nodes.top());
			ss.str("");
			ss << "Generating maze: " << visited_nodes << '/' << map_width * map_height << " (" << visited_nodes / (map_width * map_height) * 100.f << "%)";
			text.setString(ss.str());
			window.clear();
			window.draw(board);
			window.draw(text);
			rs.setPosition(current->x, current->y);
			window.draw(rs);
			window.display();
			//std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}

		for (auto& row : map)
		{
			for (Node& node : row)
			{
				node.visited = false;
			}
		}
		board.update();
		visited_nodes = 0;
		ss << std::fixed;
	}

	void generate_obstacles()
	{
		std::random_device rd;
		std::mt19937_64 twister(rd());
		std::uniform_int_distribution<int> distribution(0, 20);

		for (auto& row : map)
		{
			for (Node& node : row)
			{
				const int roll = distribution(twister);
				if (roll == 0 && &node != start && &node != stop)
				{
					node.blocking = true;
				}
			}
		}
	}

	static float distance(const Node& left, const Node& right)
	{
		return std::sqrtf(std::powf(left.x - right.x, 2) + std::powf(left.y - right.y, 2));
	}

	float heuristic(const Node& node) const
	{
		static const float d = distance(map[0][0], map[0][1]);
		return 1 * (std::fabs(node.x - stop->x) + std::fabs(node.y - stop->y));
	}

	void reset_map()
	{
		text.setPosition(map_height * (Node::sizex + Node::spacing), 5.f);
		map = Map();
		for (int i = 0; i < map_height; i++)
		{
			map.push_back(std::vector<Node>());
			for (int j = 0; j < map_width; j++)
			{
				map[i].emplace_back();
			}
		}

		for (int i = 0; i < map_height; i++)
		{
			for (int j = 0; j < map_width; j++)
			{
				map[i][j].x = i * (Node::sizex + Node::spacing);
				map[i][j].y = j * (Node::sizey + Node::spacing);
				map[i][j].gridx = i;
				map[i][j].gridy = j;
				map[i][j].neighbors.clear();
			}
		}

		for (int i = 0; i < map_height; i++)
		{
			for (int j = 0; j < map_width; j++)
			{
				if (i > 0) map[i][j].neighbors.push_back(&map[i - 1][j]);
				if (i < map_height - 1) map[i][j].neighbors.push_back(&map[i + 1][j]);
				if (j > 0) map[i][j].neighbors.push_back(&map[i][j - 1]);
				if (j < map_width - 1) map[i][j].neighbors.push_back(&map[i][j + 1]);
			}
		}

		start = &map[0][0];
		stop = &map[map_height - 1][map_width - 1];
		board.reset_board();
	}

	Map map;
	sf::RenderWindow window;
	BoardDrawer board;
	sf::Font font;
	sf::Text text;
	float total_time;
	float real_time;
	float visited_nodes;
	std::stringstream ss;
	int path_length;
	sf::Clock dt;
};

int main()
{
	AStarApp app;
	app.run();
}
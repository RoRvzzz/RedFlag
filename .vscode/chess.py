import turtle

# config
TILE_SIZE = 60
BOARD_WIDTH = TILE_SIZE * 8
BOARD_HEIGHT = TILE_SIZE * 8
X_OFFSET = -BOARD_WIDTH // 2
Y_OFFSET = -BOARD_HEIGHT // 2

COLORS = {
    'dark_sq': '#769656',  
    'light_sq': '#eeeed2', 
    'select': '#ba6b6c',   
    'bg': '#312e2b'        
}

# Shapes
def register_shapes(screen):
    screen.register_shape('king', ((-5,-8),(5,-8),(5,-3),(4,-2),(4,0),(2,1),(2,6),(4,6),(4,7),(2,7),(2,9),(-2,9),(-2,7),(-4,7),(-4,6),(-2,6),(-2,1),(-4,0),(-4,-2),(-5,-3),(-5,-8)))
    screen.register_shape('queen', ((-5,-8),(5,-8),(5,-3),(4,-2),(4,0),(2,1),(2,5),(3,6),(4,5),(5,6),(5,7),(3,8),(1,7),(0,8),(-1,7),(-3,8),(-5,7),(-5,6),(-4,5),(-3,6),(-2,5),(-2,1),(-4,0),(-4,-2),(-5,-3),(-5,-8)))
    screen.register_shape('bishop',((0,7),(1,6),(2,5),(1,4),(2,3),(1,2),(3,1),(4,0),(3,-1),(3,-3),(4,-4),(4,-8),(-4,-8),(-4,-4),(-3,-3),(-3,-1),(-4,0),(-3,1),(-1,2),(-2,3),(-1,4),(-2,5),(-1,6),(0,7)))
    screen.register_shape('knight',((-5,-8),(5,-8),(5,-3),(1,-3),(4,1),(1,5),(-2,4),(-2,0),(-5,0),(-5,-8)))
    screen.register_shape('rook', ((-4,4),(4,4),(4,2),(3,1),(3,-2),(5,-6),(5,-8),(-5,-8),(-5,-6),(-3,-2),(-3,1),(-4,2),(-4,4)))
    screen.register_shape('pawn',((0,6),(1,6),(2,5),(2,4),(2,2),(1,1),(2,0),(2,-2),(3,-3),(4,-6),(3,-7),(0,-7),(-3,-7),(-4,-6),(-3,-3),(-2,-2),(-2,0),(-1,1),(-2,2),(-2,4),(-2,5),(-1,6),(0,6)))

# classes

class Piece(turtle.Turtle):
    def __init__(self, name, team, row, col):
        super().__init__()
        self.p_name = name  # 'king', 'pawn', etc.
        self.team = team    # 'white' or 'black'
        self.row = row
        self.col = col
        self.has_moved = False
        
        self.shape(name)
        self.penup()
        self.turtlesize(2) # scale up the vector shapes
        self.left(90) # orient shapes upright
        
        # color setup
        if team == 'white':
            self.color('black', 'white') # outline, fill
        else:
            self.color('black', 'black')

        self.update_position()

    def update_position(self):
        """Moves the turtle to its logical grid position."""
        x = X_OFFSET + (self.col * TILE_SIZE) + (TILE_SIZE // 2)
        y = Y_OFFSET + (self.row * TILE_SIZE) + (TILE_SIZE // 2)
        self.goto(x, y)

class ChessGame:
    def __init__(self):
        self.screen = turtle.Screen()
        self.screen.title("Python Turtle Chess")
        self.screen.bgcolor(COLORS['bg'])
        self.screen.setup(width=600, height=600)
        self.screen.tracer(0) # disable auto-animation for speed
        
        register_shapes(self.screen)
        
        self.board_drawer = turtle.Turtle()
        self.board_drawer.hideturtle()
        self.board_drawer.speed(0)
        
        self.pieces = [] # list of piece objects
        self.grid = [[None for _ in range(8)] for _ in range(8)] # 8x8 grid
        
        self.turn = 'white'
        self.selected_piece = None
        
        self.draw_board()
        self.setup_pieces()
        
        # bind clicks
        self.screen.onclick(self.handle_click)
        
        self.update_screen()

    def draw_board(self):
        """draws the checkerboard using stamping (faster than 64 turtles)."""
        self.board_drawer.penup()
        self.board_drawer.shape("square")
        self.board_drawer.turtlesize(TILE_SIZE / 20) # default square is 20px
        
        for row in range(8):
            for col in range(8):
                x = X_OFFSET + (col * TILE_SIZE) + (TILE_SIZE // 2)
                y = Y_OFFSET + (row * TILE_SIZE) + (TILE_SIZE // 2)
                self.board_drawer.goto(x, y)
                
                color = COLORS['light_sq'] if (row + col) % 2 == 0 else COLORS['dark_sq']
                self.board_drawer.color(color)
                self.board_drawer.stamp()

    def setup_pieces(self):
        layout = [
            ['rook', 'knight', 'bishop', 'queen', 'king', 'bishop', 'knight', 'rook'],
            ['pawn'] * 8,
            [None] * 8, [None] * 8, [None] * 8, [None] * 8, # Empty rows
            ['pawn'] * 8,
            ['rook', 'knight', 'bishop', 'queen', 'king', 'bishop', 'knight', 'rook']
        ]

        for row in range(8):
            for col in range(8):
                p_type = layout[row][col]
                if p_type:
                    team = 'white' if row < 2 else 'black'
                    self.create_piece(p_type, team, row, col)

    def create_piece(self, name, team, row, col):
        p = Piece(name, team, row, col)
        self.pieces.append(p)
        self.grid[row][col] = p

    def handle_click(self, x, y):
        """converts screen pixel click to grid coordinates."""
        col = int((x - X_OFFSET) // TILE_SIZE)
        row = int((y - Y_OFFSET) // TILE_SIZE)

        # check bounds
        if 0 <= row < 8 and 0 <= col < 8:
            self.process_move_logic(row, col)

    def process_move_logic(self, row, col):
        clicked_piece = self.grid[row][col]

        # 1. select a piece
        if self.selected_piece is None:
            if clicked_piece and clicked_piece.team == self.turn:
                self.selected_piece = clicked_piece
                self.highlight_square(row, col)
            return

        # 2. deselect or switch selection
        if clicked_piece and clicked_piece.team == self.turn:
            self.selected_piece = clicked_piece
            self.draw_board() # Redraw to clear highlight
            self.highlight_square(row, col)
            self.restore_pieces()
            return

        # 3. attempt move
        if self.is_legal_move(self.selected_piece, row, col):
            self.move_piece(self.selected_piece, row, col)
            
            # switch turn
            self.turn = 'black' if self.turn == 'white' else 'white'
            print(f"{self.turn.capitalize()}'s turn")
            
            # reset selection
            self.selected_piece = None
            self.draw_board()
            self.restore_pieces()
        else:
            print("Invalid Move")

    def highlight_square(self, row, col):
        # simply redraw board with a different color at that spot
        self.draw_board() 
        self.board_drawer.goto(X_OFFSET + col*TILE_SIZE + TILE_SIZE//2, Y_OFFSET + row*TILE_SIZE + TILE_SIZE//2)
        self.board_drawer.color(COLORS['select'])
        self.board_drawer.stamp()
        self.restore_pieces() # redraw pieces on top

    def restore_pieces(self):
        # turtle stamps get covered by pieces, but pieces are actual turtles
        # we just need to ensure the screen updates
        self.update_screen()

    def move_piece(self, piece, target_row, target_col):
        # capture logic
        target_piece = self.grid[target_row][target_col]
        if target_piece:
            target_piece.hideturtle()
            self.pieces.remove(target_piece)

        # update grid
        self.grid[piece.row][piece.col] = None
        self.grid[target_row][target_col] = piece

        # update piece internal state
        piece.row = target_row
        piece.col = target_col
        piece.has_moved = True
        piece.update_position()

    def is_legal_move(self, piece, tr, tc):
        """checks if a move from piece.row, piece.col to tr, tc is valid."""
        pr, pc = piece.row, piece.col
        
        # cannot land on own team
        target = self.grid[tr][tc]
        if target and target.team == piece.team:
            return False

        dx = tc - pc
        dy = tr - pr
        
        # pawn logic
        if piece.p_name == 'pawn':
            direction = 1 if piece.team == 'white' else -1
            start_row = 1 if piece.team == 'white' else 6
            
            # move forward 1
            if dx == 0 and dy == direction and target is None:
                return True
            # move forward 2
            if dx == 0 and dy == 2*direction and pr == start_row and target is None and self.grid[pr+direction][pc] is None:
                return True
            # capture
            if abs(dx) == 1 and dy == direction and target is not None:
                return True
            return False

        # knight logic
        if piece.p_name == 'knight':
            return (abs(dx), abs(dy)) in [(1, 2), (2, 1)]

        # king logic
        if piece.p_name == 'king':
            return max(abs(dx), abs(dy)) == 1

        # sliding logic (rook, bishop, queen)
        if piece.p_name in ['rook', 'bishop', 'queen']:
            # check geometry
            straight = (dx == 0 or dy == 0)
            diagonal = (abs(dx) == abs(dy))
            
            if piece.p_name == 'rook' and not straight: return False
            if piece.p_name == 'bishop' and not diagonal: return False
            if piece.p_name == 'queen' and not (straight or diagonal): return False

            # check path for obstructions
            step_x = 0 if dx == 0 else dx // abs(dx)
            step_y = 0 if dy == 0 else dy // abs(dy)
            
            cur_r, cur_c = pr + step_y, pc + step_x
            while (cur_r != tr or cur_c != tc):
                if self.grid[cur_r][cur_c] is not None:
                    return False # path blocked
                cur_r += step_y
                cur_c += step_x
            return True

        return False

    def update_screen(self):
        self.screen.update()

# main loop
if __name__ == "__main__":
    game = ChessGame()
    turtle.done()
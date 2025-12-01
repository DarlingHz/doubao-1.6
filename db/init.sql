-- 创建用户表
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    nickname TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 创建影片表
CREATE TABLE IF NOT EXISTS movies (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    genres TEXT NOT NULL,  -- JSON格式存储类型数组
    duration INTEGER NOT NULL,  -- 总时长，单位分钟
    status INTEGER DEFAULT 1,  -- 1: 活跃, 0: 已删除
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 创建观影记录表
CREATE TABLE IF NOT EXISTS watch_records (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    movie_id INTEGER NOT NULL,
    start_time TIMESTAMP NOT NULL,
    watch_duration INTEGER NOT NULL,  -- 观看时长，单位分钟
    is_completed BOOLEAN DEFAULT FALSE,
    rating INTEGER,  -- 评分，1-10
    comment TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id),
    FOREIGN KEY (movie_id) REFERENCES movies(id)
);

-- 创建索引
CREATE INDEX IF NOT EXISTS idx_users_nickname ON users(nickname);
CREATE INDEX IF NOT EXISTS idx_movies_title ON movies(title);
CREATE INDEX IF NOT EXISTS idx_movies_status ON movies(status);
CREATE INDEX IF NOT EXISTS idx_watch_records_user_id ON watch_records(user_id);
CREATE INDEX IF NOT EXISTS idx_watch_records_movie_id ON watch_records(movie_id);
CREATE INDEX IF NOT EXISTS idx_watch_records_user_movie ON watch_records(user_id, movie_id);
CREATE INDEX IF NOT EXISTS idx_watch_records_start_time ON watch_records(start_time);
